# Ubuntu / WSL Ubuntu Setup

This guide starts from a fresh Ubuntu or WSL Ubuntu checkout. CampusCompute's required engine is C/POSIX software; the React dashboard and FastAPI adapter are optional local-development components.

## 1. Install prerequisites

Update apt and install the C toolchain, Python tooling, and Node.js package prerequisites:

```bash
sudo apt update
sudo apt install -y build-essential make coreutils git python3 python3-venv python3-pip nodejs npm
```

Check the tools:

```bash
gcc --version
make --version
python3 --version
node --version
npm --version
```

The frontend CI currently builds with Node.js 20. If your Ubuntu package provides an older Node.js release, install Node.js 20 before building the dashboard. The C engine itself does not require Python or Node.js.

## 2. Clone and enter the repository

```bash
git clone https://github.com/nischalk987/CampusCompute.git
cd CampusCompute
```

If you already have the repository, enter its root directory and optionally update it:

```bash
git pull --ff-only
```

## 3. Clean build the C engine

From the repository root:

```bash
make clean build
```

This builds the standalone modules and the integrated executable at `engine/campuscompute`. The build uses GCC with C11, warnings enabled, and warnings treated as errors.

If the build fails, see [Troubleshooting](TROUBLESHOOTING.md).

## 4. Run the required CLI

```bash
./engine/campuscompute
```

The main menu provides FR1 through FR7 and option 0 to exit. See the [User Guide](USER_GUIDE.md) for menu choices and input order.

## 5. Run automated tests

```bash
make test-engine
```

To run the backend tests too, create and activate a virtual environment, install the backend requirements, then run the root test target:

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r backend/requirements.txt
make test
```

The root `make test` runs the C engine suite and FastAPI tests. It does not build the React frontend.

## 6. Optional: start the FastAPI adapter

In a terminal from the repository root:

```bash
source .venv/bin/activate
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
```

Open the local API docs at http://127.0.0.1:8000/docs. Keep the service bound to `127.0.0.1`; it is designed as a local development adapter, not a public internet service.

The adapter exposes health/module information, a real FR3 producer–consumer API operation, and restricted local terminal-session routes. It is not a full structured API for FR1–FR8.

## 7. Optional: start the React dashboard

Open a second terminal:

```bash
cd CampusCompute/frontend
npm install
npm run dev
```

Open the local URL printed by Vite. The dashboard can connect to the local API where supported. The C CLI remains the authoritative interface for the complete FR1–FR8 demonstrations.

For a production bundle:

```bash
npm run build
npm run preview
```

## 8. Cleanup

From the repository root:

```bash
make clean
```

This removes generated C executables and backend Python test caches. It does not delete source files or the virtual environment. To remove the local Python environment separately, run `rm -rf .venv`.

Runtime logs are written under `logs/`. Keep any output you need before removing generated logs. Do not manually unlink shared-memory objects or semaphores while an FR4 writer/reader pair is running.
