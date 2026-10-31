# CPU Scheduler Command Guide

Build the updated scheduler:
```bash
make -C engine/src/scheduler clean all
```
Run:
```bash
./engine/src/scheduler/cpu_scheduler
```
Enter jobs with ID, arrival time, burst time, and Priority / queue class. Menu options:
- 2 FCFS
- 3 Round Robin (asks for time quantum)
- 4 Non-preemptive Priority
- 5 SJF
- 6 SRTF
- 7 Multilevel Queue (Priority/queue class: 0 foreground, 1 interactive, 2+ batch)
- 8 Compare all six policies

Run tests:
```bash
make -C engine/src/scheduler test
```
