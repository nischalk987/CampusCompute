# FR2 — CPU Scheduling

This standalone C module implements FCFS, Round Robin, and non-preemptive Priority scheduling using the FR1 `JobManager` data model.

## Scheduling rules
- **FCFS:** increasing arrival time; equal arrival times use original insertion order.
- **Round Robin:** FIFO ready queue; new jobs arriving during a time slice are enqueued before the unfinished job is requeued. Equal arrival times use insertion order.
- **Non-preemptive Priority:** among arrived jobs, lower numeric priority means higher priority. Ties use earlier arrival time, then insertion order. A selected job runs to completion.
- CPU idle gaps are represented by timeline gaps rather than fake execution slices.

## Metrics
- Completion time (CT): time when the job finishes.
- Turnaround time (TAT): CT − arrival time.
- Waiting time (WT): TAT − CPU burst.
- Outputs show each job's metrics, average TAT, average WT, timeline, and makespan.

## Build and test
From repository root on Linux/Ubuntu or WSL Ubuntu:
```bash
make -C engine/src/scheduler clean all
make -C engine/src/scheduler test
```

## Interactive CLI
```bash
./engine/src/scheduler/cpu_scheduler
./engine/src/scheduler/cpu_scheduler --help
```
Choose option 1 to enter jobs, then run FCFS, Round Robin, Priority, or compare them. Round Robin quantum must be a positive integer.

## Sample FCFS case
- P1: arrival 0, burst 5, priority 2
- P2: arrival 1, burst 3, priority 1
- P3: arrival 8, burst 2, priority 3

Expected FCFS completion times are 5, 8, 10; average turnaround time is 4.67 and average waiting time is 1.33.
