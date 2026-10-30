# CampusCompute

**CampusCompute — Intelligent Job Scheduling and Resource Management**

CampusCompute is an Operating Systems Level 3 mini-project for demonstrating core OS concepts through a command-line application written in C on Linux/Ubuntu or WSL Ubuntu. A dashboard-style interface may be added to explore results, but required OS operations and algorithm outputs must come from working code—not hard-coded demonstrations.

> **Project status:** This repository is being initialized. The architecture and commands below describe the intended implementation; verify each feature against the source code as it is added.

## Objectives

- Manage jobs and demonstrate process lifecycle concepts.
- Implement and compare FCFS, Round Robin, and non-preemptive Priority CPU scheduling.
- Demonstrate synchronized producer–consumer logging with POSIX threads.
- Exchange data between processes using POSIX shared memory and semaphore synchronization.
- Check resource allocation safety with the Banker's Algorithm.
- Detect normal and controlled simulated worker failures.
- Demonstrate a process resource limit and clean up program-created resources.
- Provide a documented CLI with input validation and useful error messages.

## Architecture

**Detailed architecture diagrams:** [Open ARCHITECTURE.md](ARCHITECTURE.md) for the system-context diagram, layered component architecture, module responsibilities, scheduling data flow, dashboard request sequence, and cleanup/error-handling flow.

The planned system has three layers:

1. **Frontend dashboard (optional UI):** Overview cards, scheduling results, IPC activity, resource-safety analysis, process status, and terminal-style interaction.
2. **Integration layer (optional):** A local service may validate requests, invoke the compiled C executable, and return parsed results. It must safely validate inputs and must not expose arbitrary command execution.
3. **C OS engine (required):** Core logic executes on Linux/Ubuntu or WSL Ubuntu and produces real results. The CLI must be independently testable.

### Planned module map

| Requirement | Module | Expected demonstration |
|---|---|---|
| FR1 | Job manager | Job ID, arrival time, CPU burst, priority, status, validation and lifecycle |
| FR2 | CPU scheduler | FCFS, Round Robin, non-preemptive Priority, timeline, completion/turnaround/waiting times and averages |
| FR3 | Producer–Consumer logger | Bounded queue, POSIX threads, mutex and condition-variable synchronization, readable event log |
| FR4 | Shared-memory IPC | Cooperating processes exchange data through POSIX shared memory and a named semaphore or suitable POSIX synchronization |
| FR5 | Banker's Algorithm | Available, Allocation, Maximum and Need matrices; safe sequence or unsafe-state result |
| FR6 | Worker process monitor | `fork()` and `waitpid()` (or documented equivalent), normal completion and controlled simulated failure |
| FR7 | Process limits and cleanup | At least one supported resource limit using `setrlimit()` or a suitable Linux mechanism, observed effect and cleanup |
| FR8 | CLI and error handling | Documented commands/menu, input validation and useful error/usage messages |

## Repository structure

The following is a proposed layout; update it to match the actual source files as implementation progresses.

```text
CampusCompute/
├── frontend/                 # Optional dashboard UI
│   └── src/
│       ├── components/
│       ├── layouts/
│       ├── pages/
│       └── services/          # API client
├── backend/                  # Optional local integration service
│   ├── main.py
│   ├── routes/
│   └── services/
├── engine/                   # Required C implementation
│   ├── include/
│   ├── src/
│   │   ├── main.c
│   │   ├── job_manager.c
│   │   ├── scheduler.c
│   │   ├── producer_consumer.c
│   │   ├── shared_memory.c
│   │   ├── banker.c
│   │   └── process_limits.c
│   └── Makefile
├── logs/                     # Generated runtime logs; avoid committing secrets
├── tests/
├── README.md
└── .gitignore
```

## Requirements

- Linux/Ubuntu or WSL Ubuntu.
- GCC (or another compatible C compiler).
- GNU Make, if using the Makefile.
- POSIX threads and POSIX IPC support available from the target Linux environment.
- Python and Node.js are only needed if the optional backend/frontend are implemented.

Check your toolchain:

```bash
gcc --version
make --version
```

## Build and run

The exact build command depends on the source files and Makefile committed to the repository. Once the C engine is present, a typical workflow is:

```bash
# From the repository root
make -C engine
./engine/<compiled-program> --help
```

Replace the placeholder executable and commands with the actual names defined by the implementation. These placeholder commands are not usable until the corresponding Makefile and program exist.

If the implementation offers a menu instead of command flags, run the compiled program and follow the usage instructions it displays.

## Scheduling definitions

Run all policies on the same validated job set for meaningful comparison.

- **FCFS:** Execute jobs in arrival order. Document how equal arrival times are resolved.
- **Round Robin:** Execute ready jobs for a fixed time quantum; document queue behavior and the quantum used.
- **Non-preemptive Priority:** Choose the highest-priority ready job and run it to completion. Document whether a lower or higher numeric value means higher priority and state the tie-breaking rule.

For every policy, calculate each job's completion time, turnaround time and waiting time, plus averages. Generate any timeline/Gantt chart from computed execution intervals.

## Validation and cleanup

- Reject malformed inputs and report valid usage.
- Check return values from system calls and library functions.
- Join worker threads and reap child processes.
- Unlink only shared-memory objects and semaphores created by this program.
- Close file descriptors and release allocated memory.
- Distinguish a deliberately simulated worker failure from a real OS failure.
- Do not execute arbitrary user-supplied shell commands through a web integration layer.

## Evidence and submission checklist

The course specification calls for genuine terminal screenshots numbered S01 onward, each with one explanatory sentence, combined into a single PDF named using the student's own SRN. Evidence should show the actual full SRN in the terminal prompt, commands, and meaningful output. Do not fabricate screenshots or include features that are not implemented.

- **S01–S05:** Environment, toolchain, project files, clean build and CLI usage.
- **S06–S09:** FCFS, Round Robin, Priority and scheduling comparison.
- **S10–S11:** Producer–consumer activity and orderly thread synchronization/termination.
- **S12–S13:** Shared-memory IPC, semaphore coordination and cleanup.
- **S14–S15:** Banker's safe and unsafe states.
- **S16–S17:** Normal worker completion and simulated failure detection.
- **S18–S20:** Resource-limit demonstration, invalid input handling and cleanup verification.
- **S21–S24:** Integrated execution, logs, final source/build files and ZIP integrity check.

Follow the instructor's current submission instructions and include only evidence for features actually implemented.

## Development roadmap

1. Initialize the C CLI, headers and Makefile; achieve a clean build.
2. Implement job management and scheduling; test all three policies with repeatable input.
3. Add the synchronized producer–consumer logger.
4. Add shared-memory IPC and semaphore cleanup.
5. Implement Banker's safety checks for safe and unsafe cases.
6. Add worker status detection and a resource-limit demonstration.
7. Add validation, error paths, tests and integrated execution.
8. Add the optional dashboard and integration service after the C modules have stable inputs and outputs.
9. Capture authentic terminal evidence and package the final source/build files.



## FR3 — POSIX producer–consumer activity logger

The first implemented OS module lives in `engine/src/producer_consumer/`. It uses a fixed-capacity circular buffer protected by a POSIX mutex and two condition variables. A producer thread queues activity events; a consumer thread drains the queue, prints timestamped entries, and writes them to a tab-separated log. Shutdown signals the consumer, drains pending entries, joins both threads, and reports produced/consumed counts.

### Build and test FR3

Run on Linux/Ubuntu or WSL Ubuntu with GCC and GNU Make:

```bash
make -C engine/src/producer_consumer clean all
make -C engine/src/producer_consumer test
```

Run the default 12 events, or choose a count and log file:

```bash
mkdir -p logs
./engine/src/producer_consumer/producer_consumer
./engine/src/producer_consumer/producer_consumer 25 logs/activity.log
./engine/src/producer_consumer/producer_consumer --help
```

The event count must be an integer from 1 through 100000. The log path's parent directory must already exist and be writable. The included test target checks help output, a successful 25-event run, log row count, and rejection of invalid counts. This implements FR3's producer–consumer demonstration; it does not claim that FR1–FR8 are complete.



## FR1 — Job manager and process lifecycle

Implemented C job manager files are under `engine/src/job_manager/`. The module validates unique job IDs, arrival times, CPU bursts, and priority values; stores job lifecycle state; and exposes a header for scheduler integration. Supported transitions are `CREATED -> READY -> RUNNING -> COMPLETED`. Invalid state transitions and invalid job data are rejected without modifying the job list.

Build and run the FR1 module on Linux/Ubuntu or WSL Ubuntu:

```bash
make -C engine/src/job_manager clean all
make -C engine/src/job_manager test
./engine/src/job_manager/job_manager
```

The test target covers duplicate/malformed IDs, invalid numeric ranges, lifecycle transitions, completion-time validation, and a menu smoke test. This is an independently built module; the full FR1–FR8 integrated CLI is still being assembled.



## FR2 — CPU scheduling

The standalone scheduler is under `engine/src/scheduler/` and uses the FR1 job-management module. It implements FCFS, Round Robin, and non-preemptive Priority scheduling. Results include a computed execution timeline, completion time (CT), turnaround time (TAT), waiting time (WT), average TAT/WT, and makespan.

Build and test on Linux/Ubuntu or WSL Ubuntu:

```bash
make -C engine/src/scheduler clean all
make -C engine/src/scheduler test
./engine/src/scheduler/cpu_scheduler
```

In the CLI, enter a job set once and select a policy or compare all three. Round Robin requires a positive integer quantum. Tie-breaking is documented in [the FR2 module README](engine/src/scheduler/README.md). This module is independently runnable and has not yet been integrated into one top-level FR1–FR8 executable.



## FR5 — Banker's Algorithm

The standalone Banker module is under `engine/src/banker/`. It accepts Available, Allocation, and Maximum inputs; derives `Need = Maximum - Allocation`; validates dimensions and Allocation ≤ Maximum; and reports SAFE with a computed sequence or UNSAFE when no complete safe sequence exists.

Build and test on Linux/Ubuntu or WSL Ubuntu:

```bash
make -C engine/src/banker clean all
make -C engine/src/banker test
./engine/src/banker/banker
```

The test cases include a safe state, an unsafe state, invalid allocation values, and invalid matrix dimensions. This module is standalone and not yet part of a single top-level FR1–FR8 executable.

## Academic use

This project is intended for operating-systems learning and demonstration. Document assumptions, tie-breaking rules, test inputs and observed behavior so the implementation can be explained during evaluation.

## Dashboard UI (React + Vite)

The optional CampusCompute dashboard is available under `frontend/`. It follows the approved white/soft-grey/indigo/teal palette and includes responsive navigation for all eight OS module areas.

Run locally with Node.js 20+:

```bash
cd frontend
npm install
npm run dev
```

Create and preview a production build with `npm run build` and `npm run preview`. See [frontend/README.md](frontend/README.md) for scope and data-integrity details.

**Important:** the optional dashboard is connected to the localhost API for the FR3 Producer–Consumer operation. FR1, FR2 and FR5 remain CLI-only for authoritative engine output; browser-side previews are labeled as previews. FR4, FR6 and FR7 are implemented in the C engine and available through the integrated CLI; these modules are not yet connected to structured dashboard API operations. The browser terminal is restricted and does not provide an unrestricted shell. Project completion percentages display “Not tracked yet” until repository-backed status data is available.

## Local frontend-to-engine integration

The optional FastAPI adapter under \`backend/\` provides a safe local API for the existing FR3 C producer–consumer executable. Follow [backend/README.md](backend/README.md) for Ubuntu/WSL setup. Start it from the repository root with:

\`\`\`bash
uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000
\`\`\`

The dashboard checks \`GET /health\` and its Producer–Consumer view can call \`POST /api/producer-consumer?event_count=N\`, showing events returned by the C engine. The adapter exposes only fixed operations, validates the count, uses \`shell=False\`, times out executions, and cleans temporary logs. There is deliberately no arbitrary command or terminal endpoint.

FR1, FR2 and FR5 remain accessible through their standalone C CLIs but currently do not have structured non-interactive API contracts. FR4 is implemented as a POSIX shared-memory IPC module and is integrated into the top-level C CLI. FR6 process monitoring, FR7 resource limits, and the FR8 integrated CLI are also present in the current engine. The optional dashboard/API does not yet expose every module. The API module inventory reports these limitations rather than fabricating results. See [ARCHITECTURE.md](ARCHITECTURE.md) for the actual request/response contract and integration sequence.
