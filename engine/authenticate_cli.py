#!/usr/bin/env python3
"""Authenticate the integrated C CLI against the web app's SQLite student store.

The password is read from stdin (a pipe created by the C parent), never passed
in argv or exposed in the process list.
"""
from __future__ import annotations

import hashlib
import hmac
import os
import sqlite3
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DB_PATH = Path(os.getenv(
    "CAMPUSCOMPUTE_AUTH_DB",
    str(ROOT / "data" / "campuscompute.sqlite3"),
))


def main() -> int:
    if len(sys.argv) != 2:
        print("CLI auth helper expects an SRN.", file=sys.stderr)
        return 2

    srn = sys.argv[1].strip().upper()
    password = sys.stdin.readline().rstrip("\r\n")
    if not srn or not password:
        print("Missing SRN or password.", file=sys.stderr)
        return 1
    if not DB_PATH.is_file():
        print("Student database not found. Register through the CampusCompute web app first.", file=sys.stderr)
        return 1

    try:
        with sqlite3.connect(DB_PATH, timeout=10) as connection:
            row = connection.execute(
                "SELECT full_name, srn, password_hash FROM students WHERE srn = ? COLLATE NOCASE",
                (srn,),
            ).fetchone()
    except sqlite3.Error as exc:
        print(f"Could not read student database: {exc}", file=sys.stderr)
        return 1

    if row is None:
        print("No account found for this SRN. Register through the web app first.", file=sys.stderr)
        return 1

    try:
        salt_hex, expected = row[2].split("$", 1)
        actual = hashlib.pbkdf2_hmac(
            "sha256", password.encode("utf-8"), bytes.fromhex(salt_hex), 310000
        ).hex()
    except (ValueError, TypeError):
        print("Stored password record is invalid.", file=sys.stderr)
        return 1
    finally:
        password = ""

    if not hmac.compare_digest(actual, expected):
        print("Incorrect SRN or password.", file=sys.stderr)
        return 1

    print(f'Authentication successful! Welcome, {row[0]} ({row[1]}).')
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
