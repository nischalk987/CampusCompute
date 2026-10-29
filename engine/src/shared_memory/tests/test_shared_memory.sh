
#!/usr/bin/env bash
set -Eeuo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROGRAM="${SCRIPT_DIR}/../shared_memory"

TMP_DIR="$(mktemp -d)"
WRITER_PID=""

cleanup() {
    if [[ -n "$WRITER_PID" ]]; then
        kill "$WRITER_PID" 2>/dev/null || true
        wait "$WRITER_PID" 2>/dev/null || true
    fi

    rm -rf -- "$TMP_DIR"
}
trap cleanup EXIT

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

if [[ ! -x "$PROGRAM" ]]; then
    fail "Executable not found. Run: make -C engine/src/shared_memory"
fi

echo "[TEST 1] Help command"
if "$PROGRAM" --help >"$TMP_DIR/help.txt" 2>&1 &&
   grep -q "Usage:" "$TMP_DIR/help.txt"; then
    echo "PASS: help"
else
    cat "$TMP_DIR/help.txt"
    fail "help command"
fi

echo "[TEST 2] Empty message rejected"
if "$PROGRAM" writer "" >"$TMP_DIR/invalid.txt" 2>&1; then
    fail "empty message was accepted"
fi

if grep -q "message must contain" "$TMP_DIR/invalid.txt"; then
    echo "PASS: empty message rejected"
else
    cat "$TMP_DIR/invalid.txt"
    fail "unexpected empty-message error"
fi

echo "[TEST 3] Writer-reader communication"

"$PROGRAM" writer "Automated IPC test" >"$TMP_DIR/writer.txt" 2>&1 &
WRITER_PID=$!

READER_STATUS=0
if "$PROGRAM" reader >"$TMP_DIR/reader.txt" 2>&1; then
    READER_STATUS=0
else
    READER_STATUS=$?
fi

if [[ "$READER_STATUS" -ne 0 ]]; then
    cat "$TMP_DIR/reader.txt"
    cat "$TMP_DIR/writer.txt"
    fail "reader exited with status $READER_STATUS"
fi

WRITER_STATUS=0
if wait "$WRITER_PID"; then
    WRITER_STATUS=0
else
    WRITER_STATUS=$?
fi
WRITER_PID=""

if [[ "$WRITER_STATUS" -ne 0 ]]; then
    cat "$TMP_DIR/writer.txt"
    fail "writer exited with status $WRITER_STATUS"
fi

if ! grep -Fq "message=Automated IPC test" "$TMP_DIR/reader.txt"; then
    cat "$TMP_DIR/reader.txt"
    fail "reader did not receive the expected message"
fi

if ! grep -Fq "reader acknowledged the message" "$TMP_DIR/writer.txt"; then
    cat "$TMP_DIR/writer.txt"
    fail "writer did not report acknowledgement"
fi

echo "PASS: writer-reader handshake"

echo "[TEST 4] IPC cleanup"

if [[ -e /dev/shm/campuscompute_fr4_shm ||
      -e /dev/shm/sem.campuscompute_fr4_ready ||
      -e /dev/shm/sem.campuscompute_fr4_ack ]]; then
    ls -l /dev/shm/ | grep campuscompute_fr4 || true
    fail "IPC resources remain after communication"
fi

echo "PASS: IPC cleanup"
echo "All shared-memory IPC tests passed."
