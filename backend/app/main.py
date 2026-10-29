"""Local allowlisted API adapter for implemented CampusCompute C modules."""
from __future__ import annotations

import os
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Annotated

from fastapi import FastAPI, HTTPException, Query, Request
from pydantic import BaseModel, Field
from fastapi.middleware.cors import CORSMiddleware

REPO_ROOT = Path(__file__).resolve().parents[2]
ENGINE_ROOT = REPO_ROOT / "engine" / "src"

app = FastAPI(
    title="CampusCompute Local API",
    version="1.0.0",
    description="Local adapter for CampusCompute C modules and a localhost-only, allowlisted terminal.",
)

allowed_origins = [
    item.strip()
    for item in os.getenv(
        "CAMPUSCOMPUTE_ALLOWED_ORIGINS",
        "http://127.0.0.1:5173,http://localhost:5173",
    ).split(",")
    if item.strip()
]
app.add_middleware(
    CORSMiddleware,
    allow_origins=allowed_origins,
    allow_credentials=False,
    allow_methods=["GET", "POST"],
    allow_headers=["Content-Type"],
)


def _executable(path: Path) -> Path:
    if path.exists() and os.access(path, os.X_OK):
        return path
    if shutil.which("make") is None:
        raise HTTPException(503, "GNU Make is required. Install build-essential on Ubuntu/WSL.")
    try:
        result = subprocess.run(
            ["make", "-C", str(path.parent), "all"],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            timeout=45,
            check=False,
            env={**os.environ, "LC_ALL": "C"},
        )
    except subprocess.TimeoutExpired:
        raise HTTPException(503, "Building the C module timed out.")
    if result.returncode != 0 or not path.exists():
        details = (result.stderr or result.stdout or "C module build failed")[-2000:]
        raise HTTPException(503, f"Unable to build module: {details}")
    return path


def _run_fixed(executable: Path, argv: list[str], timeout: int = 15) -> subprocess.CompletedProcess[str]:
    """Run a fixed executable using argv and shell=False; callers cannot choose executable."""
    try:
        result = subprocess.run(
            [str(executable), *argv],
            input="",
            capture_output=True,
            text=True,
            cwd=REPO_ROOT,
            timeout=timeout,
            check=False,
            env={**os.environ, "LC_ALL": "C"},
        )
    except subprocess.TimeoutExpired:
        raise HTTPException(504, "C module exceeded the execution time limit.")
    except OSError as exc:
        raise HTTPException(503, f"Unable to start C module: {exc.strerror or 'OS error'}.")
    if result.returncode != 0:
        details = (result.stderr or result.stdout or "C module returned a non-zero status")[-2000:]
        raise HTTPException(422, details.strip())
    return result


@app.get("/health")
def health() -> dict:
    return {
        "status": "ok",
        "service": "CampusCompute Local API",
        "engine_modules": {
            "jobs": "interactive-cli-only",
            "scheduling": "interactive-cli-only",
            "producer_consumer": "argv-enabled",
            "banker": "interactive-cli-only",
            "shared_memory": "not-implemented",
            "worker_failure": "not-implemented",
            "resource_limits": "not-implemented",
            "integrated_cli": "not-implemented",
        },
    }


@app.get("/api/modules")
def modules() -> dict:
    return {
        "modules": [
            {"id": "jobs", "status": "implemented", "api": False,
             "reason": "FR1 currently exposes an interactive CLI rather than a structured request interface."},
            {"id": "scheduling", "status": "implemented", "api": False,
             "reason": "FR2 currently exposes an interactive CLI; run it directly for authoritative scheduling."},
            {"id": "producer", "status": "implemented", "api": True,
             "operation": "POST /api/producer-consumer?event_count=12"},
            {"id": "ipc", "status": "not-implemented", "api": False},
            {"id": "banker", "status": "implemented", "api": False,
             "reason": "FR5 currently exposes an interactive CLI rather than a structured request interface."},
            {"id": "worker", "status": "not-implemented", "api": False},
            {"id": "limits", "status": "not-implemented", "api": False},
            {"id": "terminal", "status": "local-cli-guide", "api": False,
             "reason": "Arbitrary shell execution is intentionally not exposed."},
        ]
    }


@app.post("/api/producer-consumer")
def producer_consumer(event_count: Annotated[int, Query(ge=1, le=100_000)] = 12) -> dict:
    executable = _executable(ENGINE_ROOT / "producer_consumer" / "producer_consumer")
    try:
        # Isolate output and remove the temporary log regardless of request outcome.
        with tempfile.TemporaryDirectory(prefix="campuscompute-") as temp_dir:
            log_path = Path(temp_dir) / "activity.log"
            result = _run_fixed(executable, [str(event_count), str(log_path)])
            if not log_path.exists():
                raise HTTPException(502, "The C module completed without creating its activity log.")
            lines = log_path.read_text(encoding="utf-8").splitlines()
            if not lines or lines[0] != "timestamp\tsequence\tmessage":
                raise HTTPException(502, "The C module produced an invalid activity-log header.")
            events = []
            for line in lines[1:]:
                fields = line.split("\t", 2)
                if len(fields) != 3:
                    raise HTTPException(502, "The C module produced a malformed activity-log record.")
                try:
                    sequence = int(fields[1])
                except ValueError as exc:
                    raise HTTPException(502, "The C module produced a non-numeric event sequence.") from exc
                events.append({"timestamp": fields[0], "sequence": sequence, "message": fields[2]})
            if len(events) != event_count or [event["sequence"] for event in events] != list(range(1, event_count + 1)):
                raise HTTPException(502, "The C module event log did not match the requested event count.")
            summary = next(
                (line.strip() for line in result.stdout.splitlines() if line.startswith("Summary:")), ""
            )
            if not summary:
                raise HTTPException(502, "The C module did not return its expected completion summary.")
            return {
                "status": "success",
                "source": "C engine",
                "event_count_requested": event_count,
                "events_produced": len(events),
                "events": events,
                "summary": summary,
            }
    except HTTPException:
        raise
    except (OSError, ValueError) as exc:
        raise HTTPException(500, f"Unable to read C activity log: {exc}") from exc


# The terminal is intentionally restricted to local development commands. It never
# invokes a shell, accepts shell operators, or allows its working directory outside
# the repository. Keep Uvicorn bound to 127.0.0.1.
TERMINAL_COMMANDS = {
    "ls", "pwd", "cat", "head", "tail", "grep", "find", "wc", "sort", "uniq",
    "cut", "tr", "printf", "echo", "date", "uname", "whoami", "id", "which",
    "git", "make", "gcc", "g++", "cc", "python", "python3", "pytest",
    "file", "tree", "du", "df", "stat", "mkdir", "touch", "cp", "mv",
}
SHELL_OPERATORS = {"|", "||", "&", "&&", ";", ">", ">>", "<", "<<", "2>", "2>>"}


class TerminalRequest(BaseModel):
    command: str = Field(min_length=1, max_length=2000)
    cwd: str = Field(default=".", max_length=1000)


def _workspace_path(relative: str) -> Path:
    candidate = (REPO_ROOT / relative).resolve()
    try:
        candidate.relative_to(REPO_ROOT.resolve())
    except ValueError as exc:
        raise HTTPException(400, "Terminal working directory must stay inside the CampusCompute repository.") from exc
    if not candidate.exists() or not candidate.is_dir():
        raise HTTPException(400, "Working directory does not exist inside the repository.")
    return candidate


@app.post("/api/terminal/execute")
def terminal_execute(payload: TerminalRequest, request: Request) -> dict:
    origin = request.headers.get("origin", "")
    if origin not in allowed_origins:
        raise HTTPException(403, "Terminal requests are only accepted from the configured local CampusCompute frontend.")
    try:
        argv = shlex.split(payload.command, posix=(os.name != "nt"))
    except ValueError as exc:
        raise HTTPException(400, f"Could not parse command: {exc}") from exc
    if not argv:
        raise HTTPException(400, "Enter a command.")
    if any(token in SHELL_OPERATORS for token in argv) or any(
        any(op in token for op in ("&&", "||", ";", "|", ">", "<", "`", "$("))
        for token in argv
    ):
        raise HTTPException(400, "Shell operators, redirection and command chaining are disabled. Run one command at a time.")
    command = argv[0]
    if command in {"cd", "clear", "cls"}:
        if command == "cd":
            target = argv[1] if len(argv) > 1 else "."
            if len(argv) > 2:
                raise HTTPException(400, "Usage: cd [repository-relative-directory]")
            current = _workspace_path(payload.cwd)
            relative_target = (current / target).resolve()
            try:
                new_cwd = relative_target.relative_to(REPO_ROOT.resolve())
            except ValueError as exc:
                raise HTTPException(400, "Cannot change directory outside the repository.") from exc
            if not relative_target.is_dir():
                raise HTTPException(400, "Directory not found.")
            return {"stdout": "", "stderr": "", "exit_code": 0, "cwd": "." if str(new_cwd) == "." else new_cwd.as_posix()}
        return {"stdout": "", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    cwd = _workspace_path(payload.cwd)
    executable: str
    if command.startswith("./") or command.startswith(".\\"):
        target = (cwd / command).resolve()
        try:
            target.relative_to(REPO_ROOT.resolve())
        except ValueError as exc:
            raise HTTPException(400, "Executable must be inside the CampusCompute repository.") from exc
        if not target.is_file() or not os.access(target, os.X_OK):
            raise HTTPException(400, "Executable not found or not executable. Build the target first.")
        executable = str(target)
    elif command in TERMINAL_COMMANDS:
        executable = shutil.which(command) or ""
        if not executable:
            raise HTTPException(503, f"'{command}' is not installed in this environment.")
    else:
        raise HTTPException(400, f"Command '{command}' is not enabled. Use 'help' in the terminal to see supported commands.")
    # Reject obvious attempts to use absolute paths or traverse out of the workspace.
    for arg in argv[1:]:
        normalized = arg.replace("\\\\", "/")
        if normalized.startswith("/") or normalized.startswith("~") or any(part == ".." for part in normalized.split("/")):
            raise HTTPException(400, "Absolute paths and parent-directory traversal are disabled in terminal arguments.")
    try:
        result = subprocess.run(
            [executable, *argv[1:]],
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=30,
            check=False,
            shell=False,
            env={**os.environ, "LC_ALL": "C", "TERM": "dumb"},
        )
    except subprocess.TimeoutExpired as exc:
        stdout = exc.stdout.decode(errors="replace") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
        stderr = exc.stderr.decode(errors="replace") if isinstance(exc.stderr, bytes) else (exc.stderr or "")
        return {"stdout": stdout[-20000:], "stderr": (stderr + "\\nCommand timed out after 30 seconds.")[-4000:], "exit_code": 124, "cwd": payload.cwd}
    except OSError as exc:
        raise HTTPException(503, f"Could not start command: {exc}") from exc
    return {
        "stdout": result.stdout[-20000:],
        "stderr": result.stderr[-4000:],
        "exit_code": result.returncode,
        "cwd": payload.cwd,
    }
