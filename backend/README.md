# CampusCompute Local API

FastAPI is an optional local adapter. It exposes a small allowlist, not a shell, and runs on the same Ubuntu/WSL machine as the C executables.

## Setup (Ubuntu / WSL)

From the repository root:

```bash
sudo apt update
sudo apt install -y build-essential python3 python3-venv python3-pip
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r backend/requirements.txt
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
```

In another terminal, start the UI:

```bash
cd frontend
npm install
npm run dev
```

The local API documentation is at `http://127.0.0.1:8000/docs`.

## Current endpoint contract

| Method and route | Purpose | Actual support |
|---|---|---|
| `GET /health` | Adapter health and implemented-module capability states | Available |
| `GET /api/modules` | Explicit module/API capability inventory | Available |
| `POST /api/producer-consumer?event_count=12` | Run compiled FR3 executable and return parsed real log records | Available; event count 1–100000 |

Successful producer-consumer response shape:

```json
{
  "status": "success",
  "source": "C engine",
  "event_count_requested": 3,
  "events_produced": 3,
  "events": [
    {"timestamp": "runtime timestamp", "sequence": 1, "message": "OS activity event 1: job state changed"}
  ],
  "summary": "Summary: produced=3 consumed=3 queued=0"
}
```

The timestamp and event text above illustrate the schema only; runtime values come from the C module.

## Capability matrix

- FR1 job manager: implemented standalone C CLI; structured HTTP CRUD/list is not yet implemented.
- FR2 scheduler: implemented standalone C CLI; structured scheduling endpoint is not yet implemented.
- FR3 producer–consumer: API invokes the actual C executable and returns parsed log records.
- FR4 shared-memory IPC: implemented in the C engine and top-level CLI; no structured HTTP endpoint is exposed yet.
- FR5 Banker: implemented standalone C CLI; structured endpoint is not yet implemented.
- FR6 process monitoring, FR7 resource limits and FR8 integrated CLI: implemented in the C engine; no structured HTTP endpoints are exposed.
- Terminal: direct local CLI only; no shell endpoint exists.

These boundaries are deliberate. An unavailable module is reported as unavailable rather than returning fabricated results.

## Errors and lifecycle

| Condition | HTTP status | Behavior |
|---|---:|---|
| Invalid, missing, non-integer or out-of-range event count | 422 | Reject before launching the C process |
| C build or executable unavailable | 503 | Return an actionable build/tooling error |
| C operation exceeds its deadline | 504 | Report timeout |
| C process returns non-zero | 422 | Return bounded stderr/stdout detail |
| Log missing, malformed or inconsistent with requested count | 502 | Reject output instead of returning false success |
| Unhandled filesystem/read failure | 500 | Report that log processing failed |

## Security boundaries

- Fixed executable path below `engine/src`; clients cannot select a binary, working directory or argv.
- Uses `subprocess.run` with an argument array and `shell=False`.
- Event count restricted to 1–100000; execution timeout 15 seconds; build timeout 45 seconds.
- Binds to `127.0.0.1` by default and CORS allows only localhost Vite origins.
- Uses a new temporary directory per run; deletes the temporary activity log after response creation.
- No HTTP terminal, arbitrary command, shell string, or file-path API.

## Tests

From repository root with the `.venv` activated:

```bash
PYTHONPATH=backend pytest -q backend/tests
```

The suite checks actual FR3 execution, request validation, unavailable engine, timeout, non-zero engine status, malformed logs, module capability reporting, and the absence of a shell route.
