#!/usr/bin/env python3
"""
Generate C99 CRC-8 (nibble lookup, same as AceCRC::crc8_nibble)
using pycrc for pyCRUMBS parity with embedded CRUMBS firmware.
"""

import subprocess
from pathlib import Path
import sys

# Constants
MODEL = "crc-8"
ALGORITHM = "table-driven"
TABLE_IDX_WIDTH = "4"
OUT_C = Path(__file__).resolve().parents[1] / "src" / "pyCRUMBS" / "crc8_nibble.c"
OUT_H = Path(__file__).resolve().parents[1] / "src" / "pyCRUMBS" / "crc8_nibble.h"


def main():
    pycrc = "pycrc"
    try:
        subprocess.run([pycrc, "--version"], check=True, capture_output=True)
    except (FileNotFoundError, subprocess.CalledProcessError):
        sys.exit("Error: pycrc not found. Install with `pip install pycrc`.")

    print("Generating CRC-8 nibble-based C99 source...")

    cmd_common = [
        pycrc,
        "--model",
        MODEL,
        "--algorithm",
        ALGORITHM,
        "--table-idx-width",
        TABLE_IDX_WIDTH,
    ]

    subprocess.run(cmd_common + ["--generate", "c", "-o", str(OUT_C)], check=True)
    subprocess.run(cmd_common + ["--generate", "h", "-o", str(OUT_H)], check=True)

    print(f"SUCCESS: Generated:\n - {OUT_C}\n - {OUT_H}")
    print("CRC model: width=8 poly=0x07 init=0x00 refin=false refout=false xorout=0x00")


if __name__ == "__main__":
    main()
