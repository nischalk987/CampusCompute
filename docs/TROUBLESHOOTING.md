# Troubleshooting

## Build and toolchain

### `gcc: command not found` or `make: command not found`

Install the compiler and build tools:

```bash
sudo apt update
sudo apt install -y build-essential make coreutils
```

Check availability with `gcc --version` and `make --version`.

### Build fails with a compiler warning treated as an error

The engine uses strict C compiler flags, including `-Werror`. Read the first compiler diagnostic and fix the source warning rather than suppressing it. Run a clean build to avoid stale objects:

```bash
make clean build
```

### `Permission denied` when launching a binary

Build it using the repository Makefiles rather than downloading or copying an unknown executable:

```bash
make build
./engine/campuscompute
```

If the repository is on a Windows-mounted WSL path with restrictive mount options, move the checkout into the WSL Linux filesystem (for example, `~/CampusCompute`) and rebuild.

## CLI input and behavior

### The menu says to enter a whole number

Menu choices must be integers within the range printed in the prompt. For the main menu, valid choices are 0–7. Type the requested value and press Enter.

### A job is rejected

Check that the ID is unique and uses only letters, digits, underscore, or hyphen. Arrival time must be non-negative, CPU burst must be positive, and priority must be within -1,000,000 to 1,000,000. Job lifecycle changes must follow CREATED → READY → RUNNING → COMPLETED.

### Round Robin asks for a quantum

Enter a positive whole number. Zero, negative values, and non-numeric input are invalid.

### Banker's Algorithm reports an invalid state

Confirm that process count is 1–64, resource count is 1–32, every resource value is an unsigned 32-bit integer, and no Allocation entry exceeds its corresponding Maximum entry. A valid state can still be UNSAFE; that is an algorithm result, not an input error.

## Runtime files and operating-system features

### Producer–consumer cannot create the log

Create the parent directory and ensure it is writable:

```bash
mkdir -p logs
```

When using a custom standalone log path, its parent directory must already exist. Use a path you own and can write to.

### FR4 shared-memory exchange times out or reports IPC failure

Run only one writer/reader pair at a time. Ensure both child executables were built:

```bash
make -C engine/src/shared_memory clean all
```

The reader waits for the writer to create and size the shared-memory object, but startup and semaphore waits are bounded. If a process is killed unexpectedly, a named shared-memory object or semaphore may remain. First ensure no CampusCompute FR4 process is running. Inspect only the CampusCompute names:

```bash
ls -l /dev/shm | grep campuscompute_fr4
```

If stale objects remain and no FR4 process is active, remove only these exact project-owned paths:

```bash
rm -f /dev/shm/campuscompute_fr4_shm
rm -f /dev/shm/sem.campuscompute_fr4_ready
rm -f /dev/shm/sem.campuscompute_fr4_ack
```

Do not remove unrelated files from `/dev/shm`.

### Resource-limit demo reports failure

The demonstration depends on the host's `RLIMIT_NOFILE` hard limit and permission to lower the child soft limit. It runs in a child so the parent shell/application limit is not reduced. Run on a normal Ubuntu/WSL Linux environment and inspect the printed error before changing system-wide limits; do not raise hard limits just to force the demo to pass.

## Backend and dashboard

### API returns connection refused

Start the API from the repository root with the Python virtual environment activated:

```bash
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
```

Check http://127.0.0.1:8000/health. Keep the service bound to localhost.

### `No module named fastapi` or `uvicorn`

Create/activate the virtual environment and install the pinned project requirements:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r backend/requirements.txt
```

### Dashboard cannot reach the API

Keep the backend running on port 8000 and the Vite development server running in a second terminal. Use the local Vite URL printed in the terminal. The API CORS allowlist is intended for localhost development origins; do not expose the development server or API publicly.

### API says a module is unavailable

Not every engine feature has a structured HTTP endpoint. FR3 has a real producer–consumer endpoint; the other FR modules should be run through `./engine/campuscompute` unless a later release adds and documents an API contract. A dashboard preview is not a live C-engine result.

### Port 8000 or Vite port is already in use

Stop the previous development server with Ctrl+C in the terminal where it is running. If needed, choose another local port only after confirming the frontend/API configuration supports that port. Do not kill unrelated processes blindly.

## Test failures

Run the engine suite from the repository root:

```bash
make test-engine
```

For backend tests:

```bash
source .venv/bin/activate
make test-backend
```

The integrated CLI tests use timeouts; if a test fails, inspect the first failing assertion and its captured output in the CI job. Re-run after a clean build with `make clean build`.

For additional context, see [Architecture](../ARCHITECTURE.md), [User Guide](USER_GUIDE.md), and the module READMEs under `engine/src/`.
