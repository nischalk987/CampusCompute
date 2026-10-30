#!/usr/bin/env bash

set -eu

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT_ROOT="$(cd "$ENGINE_DIR/.." && pwd)"
APP="$ENGINE_DIR/campuscompute"

TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

cd "$PROJECT_ROOT"
mkdir -p logs

echo "======================================"
echo " CampusCompute FR8 CLI Tests"
echo "======================================"

# Test 1: Main menu input validation
printf '\nabc\n9\n0\n' | "$APP" > "$TEMP_DIR/menu.txt" 2>&1

grep -q "Enter a whole number between 0 and 7" "$TEMP_DIR/menu.txt"
grep -q "Exiting CampusCompute" "$TEMP_DIR/menu.txt"

echo "[PASS] Main menu input validation"

# Test 2: Job Manager
printf '1\n1\nP1\n0\n5\n2\n3\nMISSING\n0\n0\n' |
    "$APP" > "$TEMP_DIR/job_manager.txt" 2>&1

grep -q "Job added successfully" "$TEMP_DIR/job_manager.txt"
grep -q "Job ID was not found" "$TEMP_DIR/job_manager.txt"

echo "[PASS] Job Manager integration"

# Test 3: CPU Scheduler
printf '2\n1\n1\nP1\n0\n5\n2\n2\n0\n0\n' |
    "$APP" > "$TEMP_DIR/scheduler.txt" 2>&1

grep -q "FCFS" "$TEMP_DIR/scheduler.txt"
grep -q "Average waiting time" "$TEMP_DIR/scheduler.txt"

echo "[PASS] CPU Scheduler integration"

# Test 4: Shared-memory IPC integration
printf '4\\nCampusCompute IPC integration\\n0\\n' |
    "$APP" > "$TEMP_DIR/shared_memory.txt" 2>&1

grep -q "message=CampusCompute IPC integration" "$TEMP_DIR/shared_memory.txt"
grep -q "Shared-memory exchange completed successfully" "$TEMP_DIR/shared_memory.txt"

if [[ -e /dev/shm/campuscompute_fr4_shm ||
      -e /dev/shm/sem.campuscompute_fr4_ready ||
      -e /dev/shm/sem.campuscompute_fr4_ack ]]; then
    echo "FR4 IPC resources were not cleaned up" >&2
    exit 1
fi

echo "[PASS] Shared-memory IPC integration and cleanup"

# Test 5: Process Manager - normal execution
printf '6\n1\n0\n' |
    "$APP" > "$TEMP_DIR/process_normal.txt" 2>&1

grep -q "exit code: 0" "$TEMP_DIR/process_normal.txt"

echo "[PASS] Process Manager normal execution"

# Test 6: Process Manager - simulated failure
printf '6\n2\n0\n' |
    "$APP" > "$TEMP_DIR/process_failure.txt" 2>&1

grep -q "exit code: 42" "$TEMP_DIR/process_failure.txt"

echo "[PASS] Process Manager failure handling"

# Test 7: Resource Limits
printf '7\n0\n' |
    "$APP" > "$TEMP_DIR/resource_limits.txt" 2>&1

grep -q "Cleanup successful" "$TEMP_DIR/resource_limits.txt"

echo "[PASS] Resource Limits integration"

# Test 8: Clean exit on EOF
printf '' | "$APP" > "$TEMP_DIR/eof.txt" 2>&1

grep -q "CampusCompute stopped safely" "$TEMP_DIR/eof.txt"

echo "[PASS] Clean exit on EOF"

echo "======================================"
echo " All FR8 CLI tests passed!"
echo "======================================"
