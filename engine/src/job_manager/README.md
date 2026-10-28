# FR1 — Job manager and process lifecycle

Standalone C module providing the validated job record needed by FR2.

## Job fields and rules
Each job stores a unique ID, arrival time, CPU burst, priority, remaining time, completion time, and lifecycle status. IDs are 1–31 characters of letters, digits, `_` or `-`. Arrival time is 0–1,000,000,000; burst is 1–1,000,000,000; priority is -1,000,000 to 1,000,000. The manager supports up to 10,000 jobs.

Lifecycle: `CREATED -> READY -> RUNNING -> COMPLETED`. Invalid transitions and unknown IDs are rejected. Validation completes before mutation, so a failed add leaves the current set unchanged.

## Build and test
From repository root on Linux/Ubuntu or WSL Ubuntu:
```bash
make -C engine/src/job_manager clean all
make -C engine/src/job_manager test
```

## Interactive CLI
```bash
./engine/src/job_manager/job_manager
./engine/src/job_manager/job_manager --help
```
Menu adds/list jobs and changes lifecycle state. A completion time earlier than arrival plus burst is rejected.

## FR2 integration
Include `include/job_manager.h`; use `JobManager`, `Job`, find functions, and state helpers. The manager owns its dynamic array. Do not retain pointers to jobs across calls to `job_manager_add`, because a resize may move the array. Use IDs as stable references.
