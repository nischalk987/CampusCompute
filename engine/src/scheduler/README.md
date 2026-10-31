# FR2 — CPU Scheduling

The standalone C scheduler supports six algorithms using the FR1 JobManager data model:

- **FCFS:** increasing arrival time; ties use insertion order.
- **Round Robin:** FIFO ready queue; arrivals during a time slice are enqueued before the unfinished job is requeued.
- **Non-preemptive Priority:** lower numeric priority wins; ties use arrival time then insertion order.
- **SJF:** non-preemptive shortest burst among arrived jobs; ties use arrival time then insertion order.
- **SRTF:** preemptive shortest remaining time; decisions are made each time unit; ties use arrival time then insertion order.
- **Multilevel Queue:** three fixed classes based on the Priority field (0=foreground, 1=interactive, 2+=batch). Strict queue priority; foreground uses RR quantum 2, interactive uses RR quantum 4, batch uses FCFS. A lower queue runs only when all higher queues have no unfinished jobs. This is a fixed-priority multilevel queue, not multilevel feedback queue.

## Metrics
CT = completion time; TAT = CT - arrival; WT = TAT - burst. Reports include each job, average TAT, average WT, and timeline/makespan.

## Build and test
```bash
make -C engine/src/scheduler clean all
make -C engine/src/scheduler test
```

## CLI
```bash
./engine/src/scheduler/cpu_scheduler
```
Choose 1 to enter jobs, then options 2–7 to run individual policies, or 8 to compare all six. For multilevel queue, set Priority / queue class to 0, 1, or 2+.
