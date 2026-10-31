# CampusCompute Command Guide (Ubuntu / WSL)

Run these commands in an Ubuntu or WSL Ubuntu terminal. Unless a section says otherwise, start from the repository root.

## 1. Get the project

```bash
git clone https://github.com/nischalk987/CampusCompute.git
cd CampusCompute
```

If the repository is already cloned:

```bash
git pull --ff-only
```

## 2. Install build tools

```bash
sudo apt update
sudo apt install -y build-essential make coreutils git
```

## 3. Build the complete C engine

```bash
make clean build
```

The integrated executable is created at `engine/campuscompute`.

## 4. Run the integrated OS lab

```bash
./engine/campuscompute
```

Sign in when prompted, then choose a module from the main menu:

| Main-menu option | Module | What to try |
|---:|---|---|
| 1 | FR1 — Job Manager | Add a job, list jobs, and move it through READY, RUNNING and COMPLETED |
| 2 | FR2 — CPU Scheduling | Enter jobs and run FCFS, Round Robin or Priority |
| 3 | FR3 — Producer–Consumer | Enter an event count and inspect synchronized producer/consumer output |
| 4 | FR4 — Shared Memory IPC | Enter a message for the reader/writer process pair |
| 5 | FR5 — Banker's Algorithm | Enter process/resource counts, Available, Allocation and Maximum |
| 6 | FR6 — Worker Failure / Process Manager | Run normal completion or controlled child failure |
| 7 | FR7 — Resource Limits | Run the RLIMIT_NOFILE exhaustion and cleanup demonstration |
| 0 | Exit | Exit the application |

## 5. FR6 — Worker Failure standalone commands

Build the module, its test, and the standalone demonstration:

```bash
make -C engine/src/process_manager clean test all
```

Run a child that exits normally:

```bash
./engine/src/process_manager/worker_failure_demo normal
```

Run the controlled failure scenario (the child intentionally exits with code 42):

```bash
./engine/src/process_manager/worker_failure_demo failure
```

The report should show the child PID, exit status, and confirmation that the parent reaped the child with `waitpid()`. Exit code 42 is expected in the failure scenario; it does not mean the parent application crashed.

## 6. FR7 — Resource Limits standalone commands

Build the module and run its tests:

```bash
make -C engine/src/resource_limits clean test all
```

Run the resource-limit demonstration:

```bash
./engine/src/resource_limits/resource_limits_demo
```

Show usage:

```bash
./engine/src/resource_limits/resource_limits_demo --help
```

The report includes the original and applied `RLIMIT_NOFILE` limits, descriptors opened and closed, whether exhaustion was detected (`EMFILE`), and whether cleanup succeeded. The integrated implementation runs the limit experiment in a child process so the parent keeps its original resource limit.

## 7. Run all C engine tests

From the repository root:

```bash
make test-engine
```

This clean-builds the C modules, runs their tests, and runs the integrated CLI regression tests.

## 8. Optional backend and dashboard

The C engine can be built and used without the dashboard. To run the optional local API:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r backend/requirements.txt
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
```

In another terminal, start the React dashboard:

```bash
cd frontend
npm install
npm run dev
```

Keep the API bound to `127.0.0.1`; it is a local development adapter.

## 9. Clean generated C binaries

From the repository root:

```bash
make clean
```

This removes generated binaries and test artifacts; it does not remove source files.
