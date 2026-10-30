# CampusCompute evidence capture (Issue #15)

This directory defines the genuine evidence set S01–S24. Capture screenshots from a real Ubuntu/WSL terminal running the checked-out project. Do not edit terminal output, synthesize screenshots, or use mock output.

## Before capturing

1. Pull the latest `main` branch and follow [Ubuntu setup](../docs/UBUNTU_SETUP.md).
2. Run `make clean && make build && make test`; resolve failures before capturing.
3. Open a terminal with a readable font size. Keep the full command and relevant output visible.
4. If the project specification requires the student's SRN in the prompt, configure your real shell prompt to show your verified SRN. Do not type a made-up prompt.
5. Create `evidence/screenshots/` and save PNG screenshots using the exact names below.

## Screenshot checklist

| ID | Scenario to demonstrate | What must be visible |
|---|---|---|
| S01 | Clean source build | `make clean && make build` and successful compiler/build completion |
| S02 | Full automated test suite | `make test` and final passing summary |
| S03 | Application main menu | Launch `./engine/campuscompute` and show the real menu |
| S04 | FR1 valid job creation | Valid job fields and created job/status output |
| S05 | FR1 invalid job input | Invalid job ID or malformed input and program's validation message |
| S06 | FR2 FCFS scheduling | Input jobs, FCFS order/timeline and computed metrics |
| S07 | FR2 Round Robin | Input jobs, quantum, execution order and metrics |
| S08 | FR2 Priority scheduling | Input jobs, priorities, tie handling and computed metrics |
| S09 | FR3 producer-consumer | Real producer/consumer activity and orderly termination |
| S10 | FR3 generated log | Actual log file/output showing recorded events |
| S11 | FR4 shared-memory exchange | Writer/reader exchange with received message and acknowledgement |
| S12 | FR4 IPC cleanup | Terminal verification that the project's named shared-memory/semaphore objects are absent after exit |
| S13 | FR5 safe Banker state | Allocation/Maximum/Available or Need data and safe sequence |
| S14 | FR5 unsafe Banker state | Unsafe-state result for a deliberately unsafe test input |
| S15 | FR6 normal worker | Parent/child output and normal child exit status |
| S16 | FR6 simulated worker failure | Controlled failure scenario and detected non-zero/abnormal status |
| S17 | FR7 resource limit | Applied resource limit and observed effect |
| S18 | FR7 cleanup | Evidence that descriptors/resources opened by the scenario are closed/released |
| S19 | FR8 invalid menu input | Invalid menu choice and graceful error/re-prompt |
| S20 | FR8 end-of-input handling | EOF input and graceful program termination |
| S21 | CLI integration | A complete real CLI path from launch through a selected feature to return/exit |
| S22 | Backend health/API | Start backend and show successful `/health` response (only if backend dependencies are installed) |
| S23 | Frontend production build | `npm run build` succeeds in `frontend/` (only if Node dependencies are installed) |
| S24 | Final clean verification | Final `git status --short`, build/test result, and repository revision used for evidence |

For S12, inspect only the exact FR4 resource names documented in the troubleshooting guide, and only after the application has exited. Do not remove unrelated shared-memory or semaphore objects. If a scenario cannot be reproduced, leave its screenshot missing and explain the blocker; never substitute another screenshot.

## Capture

From the repository root:

```bash
mkdir -p evidence/screenshots
# Run each scenario in its own terminal as needed, then save a PNG named S01.png ... S24.png.
# Ubuntu: use the Screenshot app or press Shift+Print Screen and select the terminal area.
# WSL: use Windows Snipping Tool to capture the visible terminal.
```

Keep screenshots unaltered except for cropping surrounding desktop space. Do not crop away commands, errors, output, or relevant prompt details.

## Compile the PDF

Install the PDF dependency and run:

```bash
python3 -m pip install --user reportlab
python3 tools/generate_evidence_pdf.py --srn YOUR_VERIFIED_SRN --input-dir evidence/screenshots --output YOUR_VERIFIED_SRN.pdf
```

Replace `YOUR_VERIFIED_SRN` only with the student's confirmed SRN. The generator refuses to create a PDF if any S01–S24 screenshot is missing or unreadable. It does not generate screenshots or invent terminal output.

Review every page at 100% zoom before submission. The final PDF should be committed or submitted only after the student confirms the SRN and verifies all 24 screenshots.
