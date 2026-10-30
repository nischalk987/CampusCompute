#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
ENGINE_DIR="$(cd -- "$SCRIPT_DIR/.." && pwd)"
PROJECT_ROOT="$(cd -- "$ENGINE_DIR/.." && pwd)"
APP="$ENGINE_DIR/campuscompute"
TIMEOUT_SECONDS="${CAMPUSCOMPUTE_TEST_TIMEOUT:-20}"

TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

cd "$PROJECT_ROOT"
mkdir -p logs

run_cli() {
    timeout --foreground "${TIMEOUT_SECONDS}s" "$APP"
}

echo "======================================"
echo " CampusCompute integrated CLI tests"
echo "======================================"

# FR8: reject malformed menu choices and exit deterministically.
printf '\nabc\n9\n0\n' | run_cli > "$TEMP_DIR/menu.txt" 2>&1
grep -q "Enter a whole number between 0 and 7" "$TEMP_DIR/menu.txt"
grep -q "Exiting CampusCompute" "$TEMP_DIR/menu.txt"
echo "[PASS] FR8 main-menu validation"

# FR1: create a job and reject an unknown job ID.
printf '1\n1\nP1\n0\n5\n2\n3\nMISSING\n0\n0\n' |
    run_cli > "$TEMP_DIR/job_manager.txt" 2>&1
grep -q "Job added successfully" "$TEMP_DIR/job_manager.txt"
grep -q "Job ID was not found" "$TEMP_DIR/job_manager.txt"
echo "[PASS] FR1 job-manager integration"

# FR2: enter a job and run FCFS scheduling.
printf '2\n1\n1\nP1\n0\n5\n2\n2\n0\n0\n' |
    run_cli > "$TEMP_DIR/scheduler.txt" 2>&1
grep -q "FCFS" "$TEMP_DIR/scheduler.txt"
grep -q "Average waiting time" "$TEMP_DIR/scheduler.txt"
echo "[PASS] FR2 scheduler integration"

# FR3: execute the real producer-consumer binary and verify its summary.
printf '3\n3\n0\n' | run_cli > "$TEMP_DIR/producer_consumer.txt" 2>&1
grep -q "Summary: produced=3 consumed=3 queued=0" "$TEMP_DIR/producer_consumer.txt"
echo "[PASS] FR3 producer-consumer integration"

# FR4: verify the reader receives the writer's message and all named IPC
# resources are removed when the exchange completes.
printf '4\nCampusCompute IPC integration\n0\n' |
    run_cli > "$TEMP_DIR/shared_memory.txt" 2>&1
grep -q "message=CampusCompute IPC integration" "$TEMP_DIR/shared_memory.txt"
grep -q "Shared-memory exchange completed successfully" "$TEMP_DIR/shared_memory.txt"
if [[ -e /dev/shm/campuscompute_fr4_shm ||
      -e /dev/shm/sem.campuscompute_fr4_ready ||
      -e /dev/shm/sem.campuscompute_fr4_ack ]]; then
    echo "FR4 IPC resources were not cleaned up" >&2
    exit 1
fi
echo "[PASS] FR4 shared-memory exchange and cleanup"

# FR5: exercise a known-safe one-process state and reject Allocation > Maximum.
printf '5\n1\n1\n1\n1\n0\n1\n0\n0\n' |
    run_cli > "$TEMP_DIR/banker_safe.txt" 2>&1
grep -q "SAFE" "$TEMP_DIR/banker_safe.txt"
printf '5\n1\n1\n1\n0\n2\n1\n0\n0\n' |
    run_cli > "$TEMP_DIR/banker_invalid.txt" 2>&1
grep -q "Invalid state:" "$TEMP_DIR/banker_invalid.txt"
echo "[PASS] FR5 Banker's Algorithm safe and invalid states"

# FR6: child exit status is collected for both normal and simulated failure.
printf '6\n1\n0\n' | run_cli > "$TEMP_DIR/process_normal.txt" 2>&1
grep -q "Child exit code: 0" "$TEMP_DIR/process_normal.txt"
printf '6\n2\n0\n' | run_cli > "$TEMP_DIR/process_failure.txt" 2>&1
grep -q "Child exit code: 42" "$TEMP_DIR/process_failure.txt"
echo "[PASS] FR6 normal and failed child status"

# FR7: verify the isolated resource-limit demonstration cleans up descriptors.
printf '7\n0\n' | run_cli > "$TEMP_DIR/resource_limits.txt" 2>&1
grep -q "Cleanup successful  : YES" "$TEMP_DIR/resource_limits.txt"
echo "[PASS] FR7 resource-limit cleanup"

# FR8: EOF must not leave the interactive process waiting for more input.
printf '' | run_cli > "$TEMP_DIR/eof.txt" 2>&1
grep -q "CampusCompute stopped safely" "$TEMP_DIR/eof.txt"
echo "[PASS] FR8 clean exit on EOF"

echo "======================================"
echo " All integrated CLI tests passed."
echo "======================================"
