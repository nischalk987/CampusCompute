# FR3 — POSIX Producer–Consumer Activity Logger

This module demonstrates a bounded producer–consumer queue using POSIX threads.

## What it does

- A producer creates numbered operating-system activity events.
- A fixed-size circular buffer (8 entries) bounds queued work.
- A POSIX mutex protects queue state.
- `not_empty` and `not_full` condition variables coordinate consumer and producer waits.
- The consumer prints timestamped events and writes them to a tab-separated log file.
- Shutdown signals completion, drains queued work, joins both threads, and checks produced/consumed totals.

## Build

Run from the repository root on Linux/Ubuntu or WSL Ubuntu with GCC and GNU Make:

```bash
make -C engine/src/producer_consumer clean all
```

## Run

The default run generates 12 events and writes `logs/activity.log`. Create the log directory first:

```bash
mkdir -p logs
./engine/src/producer_consumer/producer_consumer
./engine/src/producer_consumer/producer_consumer 25 logs/activity.log
./engine/src/producer_consumer/producer_consumer --help
```

The event count must be from 1 through 100000. If a custom log path is supplied, its parent directory must already exist and be writable.

## Test

```bash
make -C engine/src/producer_consumer test
```

The repeatable checks cover help output, a successful 25-event run, expected log row count and rejection of zero, negative, non-numeric, and out-of-range event counts.

## Synchronization notes

All queue index/count transitions occur while holding the mutex. The producer waits while the queue is full; the consumer waits while it is empty and production has not finished. The consumer exits only when the producer is marked done and no events remain. This avoids polling and permits a clean drain at shutdown.
