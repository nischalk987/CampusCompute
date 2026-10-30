# FR6 — Process Manager

A standalone C module demonstrating process creation and child-process completion monitoring using POSIX system calls.

## Features

- Creates a child process using `fork()`.
- Waits for child completion using `waitpid()`.
- Handles interrupted wait operations (`EINTR`).
- Detects normal child termination.
- Captures the child's exit code.
- Detects termination by signal and records the signal number.
- Demonstrates controlled child failure using exit code 42.
- Reports errors through an error message.

## Build and Test

From the repository root on Linux/Ubuntu:

```bash
make -C engine/src/process_manager test
