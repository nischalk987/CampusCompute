# CampusCompute Local API

FastAPI is an optional local adapter. It exposes a small allowlist, not a shell, and runs on the same Ubuntu/WSL machine as the C executables.

## Setup (Ubuntu / WSL)

From the repository root:

\`\`\`bash
sudo apt update
sudo apt install -y build-essential python3 python3-venv python3-pip
python3 -m venv .venv
. .venv/bin/activate
python -m pip install -r backend/requirements.txt
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
\`\`\`

In another terminal, start the UI:

\`\`\`bash
cd frontend
npm install
npm run dev
\`\`\`

The local API documentation is at http://127.0.0.1:8000/docs.

## Current endpoints

| Method and route | Purpose | Data source |
|---|---|---|
| \`GET /health\` | Adapter health and module capability states | Actual adapter config |
| \`GET /api/modules\` | Explicit UI/API capability inventory | Integration contract |
| \`POST /api/producer-consumer?event_count=12\` | Run compiled FR3 executable and return parsed log records | Real C engine, temporary log |

FR1, FR2 and FR5 remain usable via their C CLIs but have no structured non-interactive interfaces, so this adapter does not claim to return their live results. FR4, FR6, FR7 and the integrated FR8 dispatcher are not implemented and are not exposed. There is no shell or terminal endpoint.

## Safety and lifecycle

- Executable paths are fixed beneath \`engine/src\`; the request cannot select executable paths.
- Calls use an argv array with \`shell=False\`.
- Event counts are limited to 1–100000.
- C builds have a 45-second timeout and module execution has a 15-second timeout.
- CORS defaults to localhost Vite origins; customize through \`CAMPUSCOMPUTE_ALLOWED_ORIGINS\` only for origins you control.
- Logs are written to an isolated temporary directory and removed after the request.
- The adapter binds to \`127.0.0.1\` by default. Do not expose this development service publicly.

## Tests

\`\`\`bash
python -m pip install -r backend/requirements.txt
PYTHONPATH=backend pytest -q backend/tests
\`\`\`

Tests cover capability reporting, input limits, real FR3 invocation, and the absence of arbitrary shell execution.
