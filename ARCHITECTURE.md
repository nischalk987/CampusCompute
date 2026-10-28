# CampusCompute — Architecture

This document defines the planned architecture for the CampusCompute Level 3 Operating Systems mini-project. It is an architecture specification, not a claim that all modules have already been implemented.

## 1. System context

```mermaid
flowchart TB
    U[Student / Evaluator]
    UI[CampusCompute Dashboard<br/>React UI — optional]
    CLI[CampusCompute CLI<br/>required interface]
    API[Local Integration API<br/>optional adapter]
    C[Compiled C OS Engine]
    LOG[(Runtime event logs)]
    OS[Linux / Ubuntu / WSL<br/>POSIX threads · IPC · processes · resource limits]

    U --> UI
    U --> CLI
    UI <--> API
    API -->|validated module request| C
    CLI -->|direct command / menu| C
    C <--> OS
    C --> LOG
    API -->|structured result| UI
```

**Design rule:** the C engine is the source of truth for operating-system demonstrations and computed results. The dashboard is optional and must not replace the required CLI or fake results. The integration API is optional; if implemented, it should expose only allowlisted operations rather than arbitrary shell execution.

## 2. Layered architecture

```mermaid
flowchart TB
    subgraph L1[Layer 1 — Presentation]
        OVERVIEW[System Overview]
        SCHEDUI[CPU Scheduling Lab]
        ADVISOR[Intelligent Advisor]
        PCUI[Producer–Consumer Logger]
        IPCUI[POSIX Shared Memory IPC]
        BANKUI[Banker's Algorithm]
        PROCUI[Process & Limits Lab]
        TERM[Live C Terminal Console]
    end

    subgraph L2[Layer 2 — Integration, optional for dashboard]
        ROUTES[Allowlisted API routes]
        VALID[Input validation and schemas]
        RUNNER[Controlled C process runner]
        PARSER[Exit status / stdout / stderr parser]
    end

    subgraph L3[Layer 3 — Required C engine]
        JOB[Job manager]
        SCHED[Scheduling algorithms]
        LOGGER[Producer–consumer queue and logger]
        SHM[Shared memory and semaphore IPC]
        BANK[Banker's safety algorithm]
        WORKER[fork / waitpid worker monitor]
        LIMIT[Resource limits and cleanup]
        CLIENG[CLI command dispatcher]
    end

    subgraph OS[Linux / Ubuntu / WSL OS services]
        PTHREAD[POSIX threads, mutexes, condition variables]
        POSIXIPC[POSIX shared memory and semaphores]
        PROCESS[Processes and wait status]
        RLIMIT[Resource limits]
        FILES[Log files and filesystem]
    end

    OVERVIEW --> ROUTES
    SCHEDUI --> ROUTES
    ADVISOR --> ROUTES
    PCUI --> ROUTES
    IPCUI --> ROUTES
    BANKUI --> ROUTES
    PROCUI --> ROUTES
    TERM --> ROUTES
    ROUTES --> VALID --> RUNNER --> CLIENG
    CLIENG --> JOB
    CLIENG --> SCHED
    CLIENG --> LOGGER
    CLIENG --> SHM
    CLIENG --> BANK
    CLIENG --> WORKER
    CLIENG --> LIMIT
    SCHED --> JOB
    LOGGER --> PTHREAD
    SHM --> POSIXIPC
    WORKER --> PROCESS
    LIMIT --> RLIMIT
    LOGGER --> FILES
    CLIENG -->|validated textual / structured output| PARSER
    PARSER -->|result and status| ROUTES
```

The direct CLI path runs the C engine without requiring the optional frontend or API. The diagram shows logical responsibilities; actual files can be consolidated or split as implementation evolves.

## 3. Module responsibilities and data flow

| Module | Inputs | Core responsibility | Outputs |
|---|---|---|---|
| Job manager (FR1) | Job ID, arrival time, burst, priority | Validate jobs and track lifecycle/status | Validated job set and statuses |
| Scheduling engine (FR2) | Same job set and selected policy | FCFS, Round Robin, non-preemptive Priority | Execution intervals, completion time, turnaround time, waiting time, averages |
| Producer–consumer logger (FR3) | Events to record | Bounded queue; producer/consumer threads; mutex and condition variables; orderly shutdown | Log entries and thread completion status |
| Shared-memory IPC (FR4) | Message/data payload | Exchange data between cooperating processes using POSIX shared memory and semaphore/suitable synchronization | Received payload and cleanup status |
| Banker's safety engine (FR5) | Available, Allocation, Maximum | Compute Need = Maximum − Allocation and run safety check | Safe sequence or unsafe-state result |
| Worker monitor (FR6) | Normal or controlled-failure mode | Create child process and collect status using fork()/waitpid() or documented equivalent | Accurate exit/failure classification |
| Resource limits (FR7) | Chosen supported limit and workload | Apply a limit using setrlimit() or suitable Linux mechanism and observe effect | Limit configuration, observed outcome and cleanup |
| CLI and validation (FR8) | Command/menu input | Dispatch only supported commands and handle malformed input | Usage/help, results and actionable errors |

## 4. Scheduling result flow

```mermaid
flowchart LR
    A[Validate job input] --> B[Create one shared job set]
    B --> C[Run FCFS]
    B --> D[Run Round Robin<br/>document quantum]
    B --> E[Run non-preemptive Priority]
    C --> F[Calculate per-job metrics]
    D --> F
    E --> F
    F --> G[Compare averages and timelines]
    G --> H[CLI output / optional dashboard]
```

For each policy, define tie-breaking explicitly. The same input job set should be used for comparisons, and all timeline and metric values must be calculated by the implementation.

## 5. Runtime request sequence (dashboard mode)

```mermaid
sequenceDiagram
    actor Student
    participant UI as React Dashboard
    participant API as Optional Local API
    participant C as C Engine
    participant OS as Linux OS

    Student->>UI: Select module and submit validated inputs
    UI->>API: Send allowlisted module request
    API->>API: Validate types, bounds and operation name
    API->>C: Invoke compiled engine with controlled arguments
    C->>OS: Perform calculation or OS operation
    OS-->>C: Return result / status
    C-->>API: Exit code and program output
    API->>API: Parse result and classify errors
    API-->>UI: Structured response
    UI-->>Student: Render results, metrics or error
```

If no API is implemented, the user runs the same supported operation directly through the CLI. Never expose a raw shell prompt backed by unrestricted server-side command execution.

## 6. Error handling and resource lifecycle

```mermaid
flowchart TD
    START[Receive command] --> VALID{Input valid?}
    VALID -- No --> USAGE[Show actionable usage error]
    VALID -- Yes --> EXEC[Execute selected module]
    EXEC --> CHECK{Operation successful?}
    CHECK -- No --> REPORT[Report real error and relevant status]
    CHECK -- Yes --> RESULT[Return computed result]
    REPORT --> CLEAN[Release resources created by this run]
    RESULT --> CLEAN
    CLEAN --> END[Exit or return to menu]
    USAGE --> END
```

Cleanup rules:

- Join threads and signal shutdown cleanly.
- Reap child processes and report their actual exit status.
- Unlink only shared-memory objects and semaphores created by this program.
- Close descriptors and release allocated memory.
- Keep generated logs separate from source files.
- Clearly label controlled simulated worker failures; do not describe them as real OS failures.

## 7. Proposed repository layout

```text
CampusCompute/
├── README.md
├── ARCHITECTURE.md
├── frontend/                 # optional React dashboard
│   └── src/
│       ├── components/
│       ├── layouts/
│       ├── pages/
│       └── services/
├── backend/                  # optional local integration API
│   ├── main.py
│   ├── routes/
│   └── services/
├── engine/                   # required C engine
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
├── tests/
└── logs/                     # runtime output; usually ignored by Git
```

This is a proposed structure, not a verified inventory of current files. File names and interfaces should be reconciled with the implementation before build commands are documented as final.

## 8. Implementation order

1. Build a working C CLI and Makefile first.
2. Implement and test job management and all three scheduling policies.
3. Implement the producer–consumer logger with safe thread shutdown.
4. Implement shared-memory IPC and semaphore cleanup.
5. Implement Banker's safety checks for both safe and unsafe test states.
6. Implement worker completion/failure detection and a resource-limit demonstration.
7. Add robust validation, error handling, tests and integrated CLI scenarios.
8. Add the optional API and dashboard, then connect UI modules to real engine results.
9. Capture genuine assessment evidence only after the relevant features work.
