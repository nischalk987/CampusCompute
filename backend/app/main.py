"""Local allowlisted API adapter for implemented CampusCompute C modules."""
from __future__ import annotations

import os
import shlex
import sys
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
            {"id": "terminal", "status": "local-restricted-terminal", "api": True,
             "operation": "POST /api/terminal/execute",
             "reason": "Executes allowlisted commands locally without shell operators or paths outside the repository."},
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
    "git", "make", "gcc", "g++", "cc", "python", "python3", "py", "pytest",
    "npm", "npx", "node", "file", "du", "df", "stat",
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

    # Built-ins keep common terminal commands usable on Windows and Linux.
    if command in {"help", "?"}:
        return {"stdout": """CampusCompute terminal
Navigation: pwd | ls [path] | cd <directory> | tree [path]
Files: cat <file> | head [-n N] <file> | tail [-n N] <file> | wc <file> | grep <text> <file>
Project: project | modules | build [scheduler|producer] | test | test scheduler | test producer | run producer [count]
Development: git status | python --version | npm --prefix frontend run build
Safety: one command at a time; shell pipes/chaining are disabled; paths must stay inside this repository.
The C engine uses POSIX/Linux features. Build and run it in Ubuntu/WSL.""", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command in {"project", "about"}:
        import platform
        return {"stdout": (
            "CampusCompute — Intelligent Job Scheduling and Resource Management\n"
            f"Repository: {REPO_ROOT}\nPlatform: {platform.platform()}\nPython: {sys.version.split()[0]}\n"
            "Frontend: React + Vite\nC engine: engine/src (Ubuntu/WSL recommended)\n\n"
            "Implemented: job manager, CPU scheduling, producer-consumer, Banker's Algorithm.\n"
            "Planned/not exposed: shared-memory IPC, worker failure, resource limits."
        ), "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command in {"modules", "module"}:
        return {"stdout": (
            "CampusCompute modules\n"
            "  jobs        implemented C job manager\n"
            "  scheduling  FCFS, Round Robin and Priority C scheduler\n"
            "  producer    POSIX producer-consumer event logger\n"
            "  banker      Banker's Algorithm\n"
            "  ipc         planned / not exposed by API\n"
            "  worker      planned / not exposed by API\n"
            "  limits      planned / not exposed by API\n"
        ), "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "build":
        target = argv[1].lower() if len(argv) > 1 else "all"
        if target not in {"all", "scheduler", "producer", "producer-consumer"}:
            return {"stdout": "", "stderr": "Usage: build [scheduler|producer]\n", "exit_code": 2, "cwd": payload.cwd}
        if os.name == "nt":
            return {"stdout": "", "stderr": (
                "The C engine uses POSIX/Linux features and must be built in Ubuntu/WSL.\n"
                "Run: make -C engine/src/scheduler test\n"
                "Run: make -C engine/src/producer_consumer test\n"
                "The frontend works on Windows: npm --prefix frontend run build\n"
            ), "exit_code": 2, "cwd": payload.cwd}
        if shutil.which("make") is None:
            return {"stdout": "", "stderr": "GNU Make is not installed.\n", "exit_code": 127, "cwd": payload.cwd}
        dirs = ["engine/src/scheduler", "engine/src/producer_consumer"] if target == "all" else [
            "engine/src/scheduler" if target == "scheduler" else "engine/src/producer_consumer"
        ]
        outputs = []
        for directory in dirs:
            result = subprocess.run(["make", "-C", str(REPO_ROOT / directory), "all"], cwd=REPO_ROOT, capture_output=True, text=True, timeout=60, check=False, shell=False)
            outputs.append(f"$ make -C {directory} all\n{result.stdout}{result.stderr}")
            if result.returncode:
                return {"stdout": "".join(outputs), "stderr": "", "exit_code": result.returncode, "cwd": payload.cwd}
        return {"stdout": "".join(outputs) + "C engine build completed.\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "test":
        target = argv[1].lower() if len(argv) > 1 else "backend"
        if target in {"scheduler", "producer", "producer-consumer", "all"}:
            if os.name == "nt":
                return {"stdout": "", "stderr": "C engine tests need Ubuntu/WSL:\nmake -C engine/src/scheduler test\nmake -C engine/src/producer_consumer test\n", "exit_code": 2, "cwd": payload.cwd}
            if shutil.which("make") is None:
                return {"stdout": "", "stderr": "GNU Make is not installed.\n", "exit_code": 127, "cwd": payload.cwd}
            dirs = (["engine/src/scheduler", "engine/src/producer_consumer"] if target == "all" else
                    ["engine/src/scheduler" if target == "scheduler" else "engine/src/producer_consumer"])
            outputs = []
            for directory in dirs:
                result = subprocess.run(["make", "-C", str(REPO_ROOT / directory), "test"], cwd=REPO_ROOT, capture_output=True, text=True, timeout=90, check=False, shell=False)
                outputs.append(f"$ make -C {directory} test\n{result.stdout}{result.stderr}")
                if result.returncode:
                    return {"stdout": "".join(outputs), "stderr": "", "exit_code": result.returncode, "cwd": payload.cwd}
            return {"stdout": "".join(outputs) + "C engine tests passed.\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
        result = subprocess.run([sys.executable, "-m", "pytest", "backend/tests", "-q"], cwd=REPO_ROOT, capture_output=True, text=True, timeout=90, check=False, shell=False)
        return {"stdout": result.stdout, "stderr": result.stderr, "exit_code": result.returncode, "cwd": payload.cwd}
    if command == "run" and len(argv) >= 2 and argv[1].lower() in {"producer", "producer-consumer"}:
        count = argv[2] if len(argv) > 2 else "12"
        if not count.isdigit() or not 1 <= int(count) <= 100000:
            return {"stdout": "", "stderr": "Usage: run producer [event_count], with count 1–100000.\n", "exit_code": 2, "cwd": payload.cwd}
        if os.name == "nt":
            return {"stdout": "", "stderr": "Run this POSIX-threaded C module in Ubuntu/WSL:\nmake -C engine/src/producer_consumer all\n./engine/src/producer_consumer/producer_consumer " + count + "\n", "exit_code": 2, "cwd": payload.cwd}
        executable_path = REPO_ROOT / "engine/src/producer_consumer/producer_consumer"
        if not executable_path.exists():
            return {"stdout": "", "stderr": "Executable not built. Run: build producer\n", "exit_code": 127, "cwd": payload.cwd}
        result = subprocess.run([str(executable_path), count], cwd=REPO_ROOT, capture_output=True, text=True, timeout=30, check=False, shell=False)
        return {"stdout": result.stdout, "stderr": result.stderr, "exit_code": result.returncode, "cwd": payload.cwd}

    if command == "pwd":
        return {"stdout": str(cwd) + "\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command in {"ls", "dir"}:
        target = cwd
        if len(argv) > 1 and not argv[1].startswith("-"):
            target = (cwd / argv[1]).resolve()
            try:
                target.relative_to(REPO_ROOT.resolve())
            except ValueError as exc:
                raise HTTPException(400, "Paths must stay inside the repository.") from exc
        if not target.exists():
            return {"stdout": "", "stderr": f"Path not found: {argv[1]}\n", "exit_code": 1, "cwd": payload.cwd}
        if target.is_file():
            return {"stdout": target.name + "\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
        entries = sorted(target.iterdir(), key=lambda p: (not p.is_dir(), p.name.lower()))
        return {"stdout": ("\n".join(p.name + ("/" if p.is_dir() else "") for p in entries) + "\n") if entries else "(empty directory)\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command in {"cat", "type"}:
        if len(argv) < 2:
            return {"stdout": "", "stderr": "Usage: cat <file> [file ...]\n", "exit_code": 2, "cwd": payload.cwd}
        output = []
        for name in argv[1:]:
            path = (cwd / name).resolve()
            try:
                path.relative_to(REPO_ROOT.resolve())
            except ValueError as exc:
                raise HTTPException(400, "Paths must stay inside the repository.") from exc
            if not path.is_file():
                return {"stdout": "", "stderr": f"File not found: {name}\n", "exit_code": 1, "cwd": payload.cwd}
            if path.stat().st_size > 1000000:
                return {"stdout": "", "stderr": "Files larger than 1 MB are not displayed.\n", "exit_code": 1, "cwd": payload.cwd}
            output.append(path.read_text(encoding="utf-8", errors="replace"))
        return {"stdout": "".join(output), "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command in {"head", "tail"}:
        args = argv[1:]
        number = 10
        if len(args) >= 2 and args[0] in {"-n", "--lines"}:
            try:
                number = max(0, min(10000, int(args[1])))
            except ValueError:
                return {"stdout": "", "stderr": "Line count must be an integer.\n", "exit_code": 2, "cwd": payload.cwd}
            args = args[2:]
        if len(args) != 1:
            return {"stdout": "", "stderr": f"Usage: {command} [-n N] <file>\n", "exit_code": 2, "cwd": payload.cwd}
        path = (cwd / args[0]).resolve()
        try:
            path.relative_to(REPO_ROOT.resolve())
        except ValueError as exc:
            raise HTTPException(400, "Paths must stay inside the repository.") from exc
        if not path.is_file():
            return {"stdout": "", "stderr": f"File not found: {args[0]}\n", "exit_code": 1, "cwd": payload.cwd}
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        selected = lines[:number] if command == "head" else (lines[-number:] if number else [])
        return {"stdout": "\n".join(selected) + ("\n" if selected else ""), "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "echo":
        return {"stdout": " ".join(argv[1:]) + "\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "tree":
        target = (cwd / (argv[1] if len(argv) > 1 else ".")).resolve()
        try:
            target.relative_to(REPO_ROOT.resolve())
        except ValueError as exc:
            raise HTTPException(400, "Paths must stay inside the repository.") from exc
        if not target.is_dir():
            return {"stdout": "", "stderr": "Directory not found.\n", "exit_code": 1, "cwd": payload.cwd}
        output = [target.name or "."]
        for i, path in enumerate(sorted(target.rglob("*"), key=lambda p: p.as_posix().lower())):
            if i >= 300:
                output.append("... output capped at 300 entries")
                break
            depth = len(path.relative_to(target).parts)
            if depth <= 4:
                output.append("  " * depth + path.name + ("/" if path.is_dir() else ""))
        return {"stdout": "\n".join(output) + "\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "wc":
        if len(argv) != 2:
            return {"stdout": "", "stderr": "Usage: wc <file>\n", "exit_code": 2, "cwd": payload.cwd}
        path = (cwd / argv[1]).resolve()
        try:
            path.relative_to(REPO_ROOT.resolve())
        except ValueError as exc:
            raise HTTPException(400, "Paths must stay inside the repository.") from exc
        if not path.is_file():
            return {"stdout": "", "stderr": f"File not found: {argv[1]}\n", "exit_code": 1, "cwd": payload.cwd}
        raw = path.read_text(encoding="utf-8", errors="replace")
        return {"stdout": f"{len(raw.splitlines())} {len(raw.split())} {len(raw.encode('utf-8'))} {argv[1]}\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "grep":
        if len(argv) < 3:
            return {"stdout": "", "stderr": "Usage: grep <text> <file>\n", "exit_code": 2, "cwd": payload.cwd}
        path = (cwd / argv[2]).resolve()
        try:
            path.relative_to(REPO_ROOT.resolve())
        except ValueError as exc:
            raise HTTPException(400, "Paths must stay inside the repository.") from exc
        if not path.is_file():
            return {"stdout": "", "stderr": f"File not found: {argv[2]}\n", "exit_code": 1, "cwd": payload.cwd}
        matches = [f"{line_no}: {line}" for line_no, line in enumerate(path.read_text(encoding="utf-8", errors="replace").splitlines(), 1) if argv[1] in line]
        return {"stdout": ("\n".join(matches) + "\n") if matches else "", "stderr": "", "exit_code": 0 if matches else 1, "cwd": payload.cwd}
    if command == "find":
        start = "."
        pattern = "*"
        args = argv[1:]
        if args and args[0] != "-name":
            start = args.pop(0)
        if len(args) == 2 and args[0] == "-name":
            pattern = args[1]
        elif args:
            return {"stdout": "", "stderr": "Usage: find [path] [-name pattern]\n", "exit_code": 2, "cwd": payload.cwd}
        root = (cwd / start).resolve()
        try:
            root.relative_to(REPO_ROOT.resolve())
        except ValueError as exc:
            raise HTTPException(400, "Paths must stay inside the repository.") from exc
        if not root.exists():
            return {"stdout": "", "stderr": f"Path not found: {start}\n", "exit_code": 1, "cwd": payload.cwd}
        items = root.rglob(pattern) if root.is_dir() else [root]
        found = []
        for path in items:
            found.append(path.relative_to(REPO_ROOT).as_posix())
            if len(found) >= 500:
                found.append("... output capped at 500 paths")
                break
        return {"stdout": ("\n".join(found) + "\n") if found else "(no matches)\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "date":
        from datetime import datetime
        return {"stdout": datetime.now().astimezone().isoformat(sep=" ", timespec="seconds") + "\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "whoami":
        username = os.getenv("USERNAME") or os.getenv("USER") or "unknown"
        return {"stdout": username + "\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "uname":
        import platform
        return {"stdout": f"{platform.system()} {platform.release()} {platform.machine()}\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}
    if command == "which":
        if len(argv) != 2:
            return {"stdout": "", "stderr": "Usage: which <program>\n", "exit_code": 2, "cwd": payload.cwd}
        found = shutil.which(argv[1])
        return {"stdout": (found + "\n") if found else "", "stderr": "" if found else f"{argv[1]} not found on PATH\n", "exit_code": 0 if found else 1, "cwd": payload.cwd}
    if command in {"mkdir", "touch", "cp", "mv"}:
        try:
            if command == "mkdir":
                if len(argv) != 2:
                    return {"stdout": "", "stderr": "Usage: mkdir <directory>\n", "exit_code": 2, "cwd": payload.cwd}
                (cwd / argv[1]).resolve().mkdir()
                (cwd / argv[1]).resolve().relative_to(REPO_ROOT.resolve())
            elif command == "touch":
                if len(argv) != 2:
                    return {"stdout": "", "stderr": "Usage: touch <file>\n", "exit_code": 2, "cwd": payload.cwd}
                path = (cwd / argv[1]).resolve()
                path.relative_to(REPO_ROOT.resolve())
                path.touch(exist_ok=True)
            else:
                if len(argv) != 3:
                    return {"stdout": "", "stderr": f"Usage: {command} <source> <destination>\n", "exit_code": 2, "cwd": payload.cwd}
                source = (cwd / argv[1]).resolve()
                destination = (cwd / argv[2]).resolve()
                source.relative_to(REPO_ROOT.resolve())
                destination.relative_to(REPO_ROOT.resolve())
                if not source.is_file():
                    return {"stdout": "", "stderr": "Source file not found; directory operations are disabled.\n", "exit_code": 1, "cwd": payload.cwd}
                if command == "cp":
                    shutil.copy2(source, destination)
                else:
                    source.rename(destination)
        except (OSError, ValueError) as exc:
            return {"stdout": "", "stderr": str(exc) + "\n", "exit_code": 1, "cwd": payload.cwd}
        return {"stdout": "completed\n", "stderr": "", "exit_code": 0, "cwd": payload.cwd}

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
