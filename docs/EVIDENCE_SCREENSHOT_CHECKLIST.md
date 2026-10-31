# Evidence Screenshot Checklist — CampusCompute

This checklist is based on the **Operating Systems (UE25CS242A) Level 3 Mini-Project** instructions supplied for this project.

The assignment lists **24 screenshots (S01–S24)**. Capture only evidence for features that are actually implemented and working. Do not fabricate output or use another student's screenshots.

## Before capturing screenshots

Every terminal screenshot must show the student's **complete actual SRN in the terminal prompt**, the command being run, and meaningful output.

In Ubuntu/WSL Ubuntu, configure the prompt for the current terminal session. Replace the placeholder with your own complete SRN:

```bash
export SRN='YOUR_FULL_SRN'
export PS1='[$SRN] \\w\\$ '
```

Keep this terminal open while capturing evidence. The SRN printed inside a source file or command output does not replace the required SRN in the prompt.

## Screenshot checklist

### A. Environment and build

| ID | Required evidence | Suggested one-line caption |
|---|---|---|
| S01 | Ubuntu/WSL terminal, complete SRN prompt, and project working directory. | Shows the Ubuntu environment, complete SRN prompt, and CampusCompute working directory. |
| S02 | GCC and Make versions, confirming build tools are available. | Confirms that GCC and Make are available for building the C project. |
| S03 | Project directory showing source files and Makefile. | Shows the CampusCompute source tree and build files. |
| S04 | Clean build from source without unresolved compiler errors. | Demonstrates a successful clean compilation of CampusCompute. |
| S05 | CLI help or usage showing available commands/menu options. | Shows the commands and menu options available in the project CLI. |

### B. CPU scheduling (FR2)

| ID | Required evidence | Suggested one-line caption |
|---|---|---|
| S06 | FCFS execution order and calculated scheduling metrics. | Demonstrates FCFS execution order and scheduling metrics. |
| S07 | Round Robin time quantum, execution order, and scheduling metrics. | Demonstrates Round Robin scheduling with its configured time quantum. |
| S08 | Priority scheduling, job priorities, tie-breaking rule, and metrics. | Demonstrates non-preemptive Priority scheduling and its tie-breaking rule. |
| S09 | Comparison of scheduling policies using the same job set. | Compares FCFS, Round Robin, and Priority using identical input jobs. |

### C. Producer–consumer and IPC (FR3–FR4)

| ID | Required evidence | Suggested one-line caption |
|---|---|---|
| S10 | Producer/consumer activity and generated log entries. | Shows producer–consumer activity and the generated event log. |
| S11 | Synchronized thread operation and orderly thread completion. | Demonstrates thread synchronization and clean thread termination. |
| S12 | Cooperating processes exchanging data through POSIX shared memory. | Shows data exchange between cooperating processes using shared memory. |
| S13 | Semaphore coordination and cleanup of the IPC resources created by the program. | Demonstrates semaphore-based coordination and cleanup of created IPC resources. |

### D. Banker's algorithm (FR5)

| ID | Required evidence | Suggested one-line caption |
|---|---|---|
| S14 | Safe resource state and a valid safe sequence. | Shows Banker's algorithm identifying a safe state and safe sequence. |
| S15 | Unsafe resource state identified by the algorithm. | Shows Banker's algorithm correctly identifying an unsafe state. |

### E. Process management and resource limits (FR6–FR7)

| ID | Required evidence | Suggested one-line caption |
|---|---|---|
| S16 | Child process creation and normal completion status. | Demonstrates normal child-process completion and parent-side status collection. |
| S17 | Controlled simulated worker failure and parent-side detection. | Demonstrates a controlled worker failure detected by the parent process. |
| S18 | Resource limit applied and its observed effect. | Shows the configured process resource limit and its observed effect. |
| S19 | Invalid command/input and the resulting useful error or usage message. | Demonstrates CLI input validation and the error message for invalid input. |
| S20 | Verification that created threads/processes have ended and program-created IPC resources are released. | Verifies orderly termination and cleanup of resources created by CampusCompute. |

### F. Integrated run and submission files

| ID | Required evidence | Suggested one-line caption |
|---|---|---|
| S21 | Integrated demonstration and its summary. | Shows the integrated CampusCompute demonstration and its summary. |
| S22 | Program-generated log file and a sample of its contents. | Shows a runtime-generated log file and sample event records. |
| S23 | Final source files, Makefile, and executable. | Shows the final project source, build instructions, and compiled executable. |
| S24 | Submission ZIP contents and successful archive integrity check. | Verifies the submission archive contents and ZIP integrity. |

## Capture and PDF rules

1. Use genuine screenshots captured from your own Ubuntu/WSL terminal.
2. Keep the complete actual SRN visible in the terminal prompt in every terminal screenshot.
3. Include the command and meaningful output in each screenshot.
4. Number screenshots S01, S02, … in order. Do not reuse screenshot numbers.
5. Put one short explanatory sentence directly below each screenshot.
6. Combine the screenshots and captions into **one PDF**.
7. Name the PDF using **your own SRN only**, for example `PES1UGXXXXXXX.pdf`. The example is a placeholder; do not use it as your filename.
8. Do not submit a separate project report.
9. If a feature is not implemented or not applicable, do not fabricate evidence for it. Confirm with the instructor how to handle any omitted item.
10. Submit source code and required headers/configuration files, a Makefile or exact build instructions, and a README with prerequisites and run/cleanup commands. Include the source/build ZIP if required by the course submission portal.

## Important distinction

The checklist describes what the assignment asks you to prove; it does **not** mean every screenshot has already been captured or every feature has been independently verified. Capture each item only after running the corresponding real implementation.
