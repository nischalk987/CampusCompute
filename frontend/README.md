# CampusCompute Dashboard

Optional responsive React + Vite interface for exploring CampusCompute modules. The integrated C CLI remains the authoritative assessment interface.

## Requirements

- Node.js 20+
- npm
- The FastAPI adapter is required for API-connected views.

## Run locally

From the repository root:

```bash
cd frontend
npm install
npm run dev
```

Open the local URL printed by Vite.

Create and preview a production build:

```bash
npm run build
npm run preview
```

## Backend connection

Start the FastAPI adapter in a separate terminal from the repository root:

```bash
source .venv/bin/activate
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
```

The dashboard checks the local API and can display actual FR3 Producer–Consumer output through the backend endpoint.

## Data-integrity and implementation boundaries

- FR1 job rows and browser-side scheduling/Banker's calculations are dashboard state or previews; they are not persisted to or executed by the C engine.
- FR3 Producer–Consumer can invoke the actual C executable through the local API and display parsed output.
- FR4 shared-memory IPC, FR6 process monitoring and FR7 resource limits are available through the integrated C CLI; they do not currently have structured dashboard API operations.
- FR1, FR2 and FR5 are also available through the C CLI, but do not currently have structured HTTP operations.
- Dashboard preview/sample values must not be presented as live engine results.
- Project progress is not connected to a repository-backed task feed.

The backend has restricted local terminal-session endpoints for repository executables and development commands. They are not an unrestricted remote shell; keep the API bound to `127.0.0.1`.

## Accessibility and responsive behavior

Semantic navigation, form labels, keyboard focus styling, screen-reader status messages, mobile sidebar, reduced-motion support and responsive layout are included.
