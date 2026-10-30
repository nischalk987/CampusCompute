# CampusCompute Dashboard

Responsive React + Vite interface using the approved white, soft-grey, indigo and teal palette.

## Run locally

Requires Node.js 20+ and npm.

```bash
cd frontend
npm install
npm run dev
```

Open the local URL printed by Vite. To make a production bundle:

```bash
npm run build
npm run preview
```

## Data-integrity rules

- The dashboard labels browser-side scheduling and Banker's calculations as **preview** calculations.
- Job rows added in the browser are held in React state only; they are not saved to the C engine.
- Producer–consumer runs display safe local commands because the UI cannot start local C processes without a backend adapter.
- Shared-memory IPC, worker monitoring and resource-limit screens are not yet connected to the local API. FR4, FR6 and FR7 are available through the integrated C CLI; the dashboard must continue to label these views as unavailable until real API operations are connected.
- Project progress says **Not tracked yet** because no repository-backed task/status feed is connected.
- The terminal screen copies commands only. It does not execute arbitrary commands in a server or browser.

## Current scope

This is a dashboard foundation for the independently runnable FR1, FR2, FR3 and FR5 modules. It is not a claim that the full FR1–FR8 application or a dashboard-to-engine API exists. The C CLI remains the authoritative assessment interface.

## Accessibility and responsive behavior

Semantic navigation, form labels, keyboard focus styling, screen-reader status messages, mobile sidebar, reduced-motion support, and responsive layout are included.