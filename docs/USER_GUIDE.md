# CampusCompute User Guide

## Start the application

Build from the repository root and launch the integrated C engine:

```bash
make clean build
./engine/campuscompute
```

The main menu is:

| Option | Feature | What it demonstrates |
|---:|---|---|
| 1 | Job Manager (FR1) | Add/list jobs and update lifecycle states |
| 2 | CPU Scheduler (FR2) | FCFS, Round Robin, Priority, and policy comparison |
| 3 | Producer–Consumer (FR3) | POSIX thread synchronization and event logging |
| 4 | Shared Memory IPC (FR4) | Separate reader/writer processes exchanging a message |
| 5 | Banker's Algorithm (FR5) | Safe-state analysis and safe sequence |
| 6 | Process Manager (FR6) | Normal child completion and simulated child failure |
| 7 | Resource Limits (FR7) | File-descriptor limit, exhaustion detection, and cleanup |
| 0 | Exit | Leave the application |

The integrated CLI uses numbered menu selections. Enter a whole number when prompted; invalid values are rejected and the program asks again. End-of-file exits the current menu safely.

## FR1 — Job Manager

Choose option 1.

| Submenu option | Action |
|---:|---|
| 1 | Add a job |
| 2 | List jobs |
| 3 | Mark a job READY |
| 4 | Mark a job RUNNING |
| 5 | Mark a job COMPLETED |
| 0 | Return to main menu |

When adding a job, enter:
1. Job ID (1–31 letters, digits, underscore, or hyphen; must be unique).
2. Arrival time (0 or greater).
3. CPU burst (positive integer).
4. Priority (-1,000,000 through 1,000,000).

Valid lifecycle transitions are CREATED → READY → RUNNING → COMPLETED. Unknown IDs and invalid transitions are rejected.

## FR2 — CPU Scheduler

Choose option 2.

1. Select **1. Enter/replace jobs**.
2. Enter the number of jobs (1–10,000).
3. For each job, enter ID, arrival time, positive CPU burst, and priority.
4. Select a policy: **2. FCFS**, **3. Round Robin**, **4. Priority**, or **5. Compare all policies**.
5. Round Robin and comparison require a positive integer time quantum.
6. Select **6. List jobs** to inspect the current set; **0** returns to the main menu.

The scheduler prints execution timeline and per-job completion time (CT), turnaround time (TAT), and waiting time (WT), along with averages and makespan.

Scheduling rules:
- FCFS orders by arrival time; ties preserve insertion order.
- Round Robin uses a FIFO ready queue; jobs arriving during a time slice are queued before the unfinished job is requeued.
- Priority is non-preemptive; lower numeric priority means higher priority. Ties use arrival time, then insertion order.

## FR3 — Producer–Consumer Logger

Choose option 3 and enter an event count from 1 to 100,000. The engine starts producer and consumer threads, synchronizes a bounded queue, prints events, writes a tab-separated log, joins both threads, and reports totals.

The default standalone log location is `logs/activity.log`. The parent directory must exist and be writable. The integrated menu runs the module with its default log path.

Standalone usage:

```bash
mkdir -p logs
./engine/src/producer_consumer/producer_consumer 10 logs/activity.log
```

Expected final summary resembles `Summary: produced=10 consumed=10 queued=0`. Event timestamps and message text are generated at runtime.

## FR4 — Shared-Memory IPC

Choose option 4 and enter a non-empty message. The integrated CLI launches a reader and writer as separate child processes. They exchange the message through POSIX shared memory and named semaphores; the writer waits for acknowledgment. The CLI reports success only if both child processes complete successfully.

Run one writer/reader exchange at a time. Do not start another concurrent exchange using the same named IPC resources.

## FR5 — Banker's Algorithm

Choose option 5, then option 1. Enter:
1. Number of processes (1–64).
2. Number of resource types (1–32).
3. Available value for each resource.
4. Allocation matrix values, process by process and resource by resource.
5. Maximum matrix values in the same order.

The program validates Allocation ≤ Maximum, derives Need = Maximum − Allocation, and prints SAFE with a safe sequence or UNSAFE when no complete safe sequence exists. Resource counts are unsigned 32-bit integers.

## FR6 — Process Manager

Choose option 6:
- **1** runs a normal child process and reports its PID and exit code.
- **2** runs a controlled simulated failure and reports exit code 42.

This is an intentional demonstration of child-status collection; it is not a crash of the parent application.

## FR7 — Resource Limits

Choose option 7. The demonstration runs in an isolated child process, applies a reduced `RLIMIT_NOFILE` soft limit where supported, opens descriptors until exhaustion, closes the descriptors it opened, and reports whether cleanup succeeded.

The parent process is not left with the child's reduced resource limit.

## Optional dashboard and API

The React dashboard is under `frontend/`; the FastAPI adapter is under `backend/`. Setup instructions are in [Ubuntu / WSL Setup](UBUNTU_SETUP.md), and API details are in [backend/README.md](../backend/README.md).

Current integration boundary:
- FR3 has a structured API operation that invokes the real C executable.
- FR1, FR2, FR4, FR5, FR6 and FR7 are demonstrated through the C CLI; they do not all have structured HTTP operations.
- Dashboard-only calculations or sample states must not be treated as authoritative C-engine output.
- The local terminal feature is restricted and is not a general-purpose public shell.

## Exit and cleanup

Choose **0** at the main menu to exit. The CLI also handles end-of-file as a safe stop. FR3 joins its threads; FR4 normally unlinks the named IPC resources after a successful exchange; FR6 reaps its child; FR7 closes the descriptors opened by its isolated child.

For build/runtime errors, consult [Troubleshooting](TROUBLESHOOTING.md).
