#!/usr/bin/env python3
import sys
import subprocess
from pathlib import Path

# Directory where `generate.py` lives
SCRIPT_DIR = Path(__file__).parent
GENERATE_PY = SCRIPT_DIR / "generate.py"
OUTPUT_DIR = SCRIPT_DIR / "output"

# Ensure output directory exists
OUTPUT_DIR.mkdir(exist_ok=True)

# All models + algorithms
MODELS = [
    "crc-8",
    "crc-16-ccitt",
    "crc-16-modbus",
    "crc-32",
]

ALGOTAGS = [
    "bit",
    "nibble",
    "nibblem",
    "byte",
]


def run(cmd):
    print(">>", " ".join(cmd))
    subprocess.run(cmd, check=True)


def main():
    print(f"Generating AceCRC into: {OUTPUT_DIR}")

    for model in MODELS:
        for algo in ALGOTAGS:
            # Generate header
            run(
                [
                    sys.executable,
                    str(GENERATE_PY),
                    "--model",
                    model,
                    "--algotag",
                    algo,
                    "--header",
                ]
            )

            # Move output
            hpp = SCRIPT_DIR / f"{model.replace('-', '')}_{algo}.hpp"
            if hpp.exists():
                hpp.rename(OUTPUT_DIR / hpp.name)

            # Generate source
            run(
                [
                    sys.executable,
                    str(GENERATE_PY),
                    "--model",
                    model,
                    "--algotag",
                    algo,
                    "--source",
                ]
            )

            # Move output
            cpp = SCRIPT_DIR / f"{model.replace('-', '')}_{algo}.cpp"
            if cpp.exists():
                cpp.rename(OUTPUT_DIR / cpp.name)

    print("Done.")


if __name__ == "__main__":
    main()
