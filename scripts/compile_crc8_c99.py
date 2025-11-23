#!/usr/bin/env python3
"""
Compile generated crc8_nibble.c into a shared library for pyCRUMBS.
"""

import subprocess
from pathlib import Path
import sys

SRC_DIR = Path(__file__).resolve().parents[1] / "src" / "pyCRUMBS"
C_FILE = SRC_DIR / "crc8_nibble.c"
ADAPTER_FILE = SRC_DIR / "crc8_adapter.c"
SO_FILE = SRC_DIR / "libcrc8.so"


def main():
    if not C_FILE.exists():
        sys.exit(f"Error: {C_FILE} not found. Run generate_crc8_c99.py first.")
    if not ADAPTER_FILE.exists():
        sys.exit(f"Error: {ADAPTER_FILE} not found. Ensure it is checked into the repo.")
    try:
        cmd = [
            "gcc",
            "-shared",
            "-fPIC",
            "-O2",
            "-std=c99",
            "-o",
            str(SO_FILE),
            str(C_FILE),
            str(ADAPTER_FILE),
        ]
        subprocess.run(cmd, check=True)
        print(f"SUCCESS: Built: {SO_FILE}")
    except subprocess.CalledProcessError as e:
        sys.exit(f"Build failed: {e}")


if __name__ == "__main__":
    main()
