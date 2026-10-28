"""Local allowlisted API adapter for implemented CampusCompute C modules."""
from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
from pathlib import Path
from typing import Annotated

from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware

REPO_ROOT = Path(__file__).resolve().parents[2]
ENGINE_ROOT = REPO_ROOT / "engine" / "src"

app = FastAPI(
    title="CampusCompute Local API",
    version="1.0.0",
    description="Local adapter for fixed, allowlisted C operations. No shell endpoint exists.",
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
