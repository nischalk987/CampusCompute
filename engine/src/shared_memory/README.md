# FR4 — POSIX Shared-Memory IPC

## Overview

This module demonstrates inter-process communication (IPC) between a writer process and a reader process using POSIX shared memory and named POSIX semaphores.

## Requirements

- Linux or Ubuntu
- GCC
- GNU Make
- POSIX shared-memory and semaphore support

## Build

From the repository root, run:

```bash
make -C engine/src/shared_memory clean all
```

## Usage

Display usage information:

```bash
./engine/src/shared_memory/shared_memory --help
```

Start the writer with a message:

```bash
./engine/src/shared_memory/shared_memory writer "Hello from CampusCompute"
```

In a second Ubuntu terminal, start the reader:

```bash
./engine/src/shared_memory/shared_memory reader
```

The writer publishes a message into shared memory and signals the reader using a named semaphore. The reader receives the message and signals an acknowledgment using a separate semaphore.

Run one writer and one reader at a time. The reader retries while waiting for the writer to create the shared-memory object and waits for the writer to finish sizing the region before mapping it. Semaphore waits are bounded to prevent indefinite blocking.

## Tests

Run the automated tests:

```bash
make -C engine/src/shared_memory clean test
```

The tests cover help output, rejection of an empty message, successful writer–reader communication, acknowledgment, and cleanup after a successful exchange. The integrated engine regression suite also exercises FR4 through the top-level menu.

## Limitations

- **Crash recovery:** If a process terminates unexpectedly before cleanup, named shared-memory or semaphore objects may remain and interfere with subsequent runs.
- **Concurrency:** The implementation supports one writer–reader pair at a time. Concurrent writer sessions are not supported.
- **Timeouts:** Startup retries and semaphore waits are bounded. An operation fails if the corresponding process does not become available in time.
- **Test coverage:** The automated tests do not exhaustively cover process crashes or every IPC failure path.

## Integrated CLI

FR4 is available from the CampusCompute main menu as option 4. Build the complete engine with `make -C engine`, then run `./engine/campuscompute` and select **4. Shared Memory IPC**. Enter a non-empty message; the CLI starts the reader and writer as separate child processes, waits for both, reports success only when both exit successfully, and returns to the main menu.

Run the complete engine regression suite with `make -C engine test`.

This module demonstrates POSIX shared-memory communication and semaphore synchronization for an operating-systems project. It is not intended to be a fully crash-resilient IPC service.
