# CampusCompute

**Intelligent Job Scheduling and Resource Management — Operating Systems mini-project**

CampusCompute is a Linux/Ubuntu C project demonstrating operating-system concepts through seven functional modules and an integrated command-line interface. An optional React dashboard and local FastAPI adapter are included, but the C engine is the authoritative implementation for OS demonstrations.

## Implementation status

| Requirement | Status | How to run |
|---|---|---|
| FR1 — Job manager and lifecycle | Implemented | Main menu option 1 |
| FR2 — FCFS, Round Robin, non-preemptive Priority | Implemented | Main menu option 2 |
| FR3 — POSIX producer–consumer logger | Implemented | Main menu option 3 |
| FR4 — POSIX shared-memory IPC and semaphores | Implemented | Main menu option 4 |
| FR5 — Banker's safety algorithm | Implemented | Main menu option 5 |
| FR6 — Child process monitoring and simulated failure | Implemented | Main menu option 6 |
| FR7 — Resource limit and descriptor cleanup demo | Implemented | Main menu option 7 |
| FR8 — Integrated CLI and input validation | Implemented | `engine/campuscompute` |
| FastAPI adapter | Partial | Real structured endpoint for FR3; restricted local terminal routes |
| React dashboard | Available | Some views are previews; not all FR modules have API operations |

Do not interpret a dashboard preview as authoritative engine output. FR1, FR2, FR4, FR5, FR6 and FR7 are available through the C CLI but do not all have structured HTTP operations.

## Quick start — Ubuntu / WSL Ubuntu

Install the C build tools:

```bash
sudo apt update
sudo apt install -y build-essential make coreutils git
```

Clone the repository and build:

```bash
git clone https://github.com/nischalk987/CampusCompute.git
cd CampusCompute
make clean build
```

Launch the integrated CLI:

```bash
./engine/campuscompute
```

Main menu:

| Option | Module |
|---:|---|
| 1 | FR1 — Job Manager |
| 2 | FR2 — CPU Scheduler |
| 3 | FR3 — Producer–Consumer |
| 4 | FR4 — Shared Memory IPC |
| 5 | FR5 — Banker's Algorithm |
| 6 | FR6 — Process Manager |
| 7 | FR7 — Resource Limits |
| 0 | Exit |

Run the C module tests and integrated CLI regression tests:

```bash
make test-engine
```

For the full clean-checkout walkthrough, see [Ubuntu / WSL Setup](docs/UBUNTU_SETUP.md).

## Documentation

- [Ubuntu / WSL setup](docs/UBUNTU_SETUP.md) — prerequisites, clone, build, run, tests and cleanup.
- [User guide](docs/USER_GUIDE.md) — menu choices, input formats, algorithms and expected output.
- [Troubleshooting](docs/TROUBLESHOOTING.md) — build, CLI, IPC, API and dashboard problems.
- [Architecture](ARCHITECTURE.md) — components, responsibilities, data flow and integration boundaries.
- [Backend API guide](backend/README.md) — setup, endpoints, validation, security and tests.
- [Dashboard guide](frontend/README.md) — local development and current UI scope.

## Repository layout

```text
CampusCompute/
├── engine/
│   ├── src/
│   │   ├── main.c                 # Integrated FR1–FR8 CLI
│   │   ├── job_manager/           # FR1
│   │   ├── scheduler/             # FR2
│   │   ├── producer_consumer/     # FR3
│   │   ├── shared_memory/         # FR4
│   │   ├── banker/                # FR5
│   │   ├── process_manager/       # FR6
│   │   └── resource_limits/       # FR7
│   └── Makefile
├── backend/                      # Optional FastAPI local adapter
├── frontend/                     # Optional React + Vite dashboard
├── docs/
│   ├── UBUNTU_SETUP.md
│   ├── USER_GUIDE.md
│   └── TROUBLESHOOTING.md
├── logs/                          # Runtime-generated logs
├── Makefile                       # Repository-level build/test targets
├── ARCHITECTURE.md
└── README.md
```

## Build and test commands

Run these from the repository root:

| Command | Purpose |
|---|---|
| `make build` | Build all standalone C modules and integrated CLI |
| `make test-engine` | Clean-build and run all C module plus integrated CLI tests |
| `make test-backend` | Run FastAPI tests (install backend requirements first) |
| `make test` | Run engine and backend tests |
| `make clean` | Remove generated C binaries and backend test caches |

Backend test setup:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r backend/requirements.txt
make test
```

## Optional dashboard and API

The backend and frontend are development-time additions. Start the API from the repository root after installing its requirements:

```bash
source .venv/bin/activate
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
```

In a second terminal:

```bash
cd frontend
npm install
npm run dev
```

The API health check is http://127.0.0.1:8000/health and interactive API documentation is http://127.0.0.1:8000/docs.

The FR3 endpoint `POST /api/producer-consumer?event_count=N` runs the real C producer–consumer executable and returns validated log records. Other OS modules remain accessible through the integrated C CLI unless and until structured API operations are implemented. The local terminal routes are restricted to local development; keep the API bound to `127.0.0.1` and do not expose it to the public internet.

## Technical concepts demonstrated

- Process/job lifecycle and state validation.
- CPU scheduling and CT/TAT/WT metrics.
- POSIX threads, mutexes and condition variables.
- POSIX shared memory and named semaphores.
- Banker's deadlock-avoidance safety check.
- `fork()`, `waitpid()` and exit-status handling.
- `setrlimit()` and file-descriptor cleanup.
- Input validation, deterministic tests and resource cleanup.

## Limitations

- The C engine targets Linux/Ubuntu and WSL Ubuntu; native Windows builds are not supported.
- FR4 supports one writer–reader pair at a time. Unexpected process termination can leave named IPC objects that may need manual cleanup; see [Troubleshooting](docs/TROUBLESHOOTING.md).
- The dashboard is not a complete API-driven control plane for FR1–FR8.
- The FastAPI service is a local development adapter, not a production deployment.
- Evidence screenshots and submission PDFs must be captured from genuine program runs and follow the instructor's latest naming/submission requirements.

## Academic use

CampusCompute is intended for learning and demonstrating operating-system mechanisms. Use the source, module READMEs, tests and architecture guide to explain the implementation and its limitations during evaluation.
