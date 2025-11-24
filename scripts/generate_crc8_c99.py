#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent
C99_DIR = SCRIPT_DIR / "c99_crc8"
C99_DIR.mkdir(exist_ok=True)

MODEL = "crc-8"
ALGOS = ["bit", "nibble", "nibblem", "byte"]

# Mapping algotag → pycrc valid flags
ALGO_FLAGS = {
    "bit": ["--algorithm", "bit-by-bit-fast"],
    "nibble": ["--algorithm", "table-driven", "--table-idx-width", "4"],
    "nibblem": ["--algorithm", "table-driven", "--table-idx-width", "4"],
    "byte": ["--algorithm", "table-driven", "--table-idx-width", "8"],
}


def run(cmd):
    print(">>", " ".join(cmd))
    subprocess.run(cmd, check=True)


def main():
    print("Generating pure C99 CRC-8 variants...")

    for algo in ALGOS:
        fileroot = f"crc8_{algo}"
        flags = ALGO_FLAGS[algo]

        # Generate .h
        run(
            [
                sys.executable,
                "-m",
                "pycrc",
                "--model",
                MODEL,
                "--std",
                "C99",
                *flags,
                "--generate",
                "h",
                "-o",
                str(C99_DIR / f"{fileroot}.h"),
            ]
        )

        # Generate .c
        run(
            [
                sys.executable,
                "-m",
                "pycrc",
                "--model",
                MODEL,
                "--std",
                "C99",
                *flags,
                "--generate",
                "c",
                "-o",
                str(C99_DIR / f"{fileroot}.c"),
            ]
        )

    print("\nDone. C99 code stored in:", C99_DIR)


if __name__ == "__main__":
    main()
