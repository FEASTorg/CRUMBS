#!/usr/bin/env python3
"""Validate and stage CRC-8 generated files from scripts/dist into src/crc.

This script copies the generated files from scripts/dist/crc/c99 and
scripts/dist/crc/arduino into src/crc/c99 and src/crc/arduino respectively.

It performs minimal checks and will create target directories if needed.
"""
from pathlib import Path
import shutil
import sys


SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent
# Use the repo-root 'dist' folder (e.g. <repo-root>/dist/crc/...) so generated
# artifacts appear in a stable, predictable location.
DIST_ROOT = REPO_ROOT / "dist" / "crc"
SRC_ROOT = Path(__file__).resolve().parents[1] / "src" / "crc"


def copy_tree(src: Path, dst: Path):
    if not src.exists():
        print(f"Source path not found: {src}")
        return 1
    dst.mkdir(parents=True, exist_ok=True)
    copied = 0
    for p in sorted(src.iterdir()):
        if p.is_file():
            target = dst / p.name
            shutil.copy2(p, target)
            print(f"Copied {p} -> {target}")
            copied += 1
    print(f"{copied} files copied from {src} to {dst}")
    return 0




def stage_variations(algos: list[str]) -> int:
    """Stage only the listed crc8 variations into src/crc.

    Examples of algo tokens: bit, nibble, nibblem, byte
    """
    rc = 0

    # c99 staging - copy only matching files for each algo
    c99_src = DIST_ROOT / "c99"
    c99_dst = SRC_ROOT / "c99"
    c99_dst.mkdir(parents=True, exist_ok=True)
    copied = 0
    for algo in algos:
        prefix = f"crc8_{algo}"
        if not c99_src.exists():
            print(f"Source path not found: {c99_src}")
            rc = 1
            break
        for p in sorted(c99_src.iterdir()):
            # Match exact stem (name without extension) to avoid 'nibble' matching 'nibblem'
            if p.is_file() and p.stem == prefix:
                target = c99_dst / p.name
                shutil.copy2(p, target)
                print(f"Copied {p} -> {target}")
                copied += 1
    print(f"{copied} files copied from {c99_src} to {c99_dst}")

    # arduino staging - copy only matching files for each algo
    arduino_src = DIST_ROOT / "arduino"
    arduino_dst = SRC_ROOT / "arduino"
    arduino_dst.mkdir(parents=True, exist_ok=True)
    copied = 0
    for algo in algos:
        prefix = f"crc8_{algo}"
        if not arduino_src.exists():
            print(f"Source path not found: {arduino_src}")
            rc = 1
            break
        for p in sorted(arduino_src.iterdir()):
            # Match exact stem to avoid overlap between variations (e.g. nibble/nibblem)
            if p.is_file() and p.stem == prefix:
                target = arduino_dst / p.name
                shutil.copy2(p, target)
                print(f"Copied {p} -> {target}")
                copied += 1
    print(f"{copied} files copied from {arduino_src} to {arduino_dst}")

    return rc


def main():
    import argparse
    import os

    parser = argparse.ArgumentParser(
        description="Stage generated CRC-8 outputs from dist into src/crc"
    )
    parser.add_argument(
        "--algos",
        help=(
            "Comma-separated list of crc8 variations to stage (bit,nibble,nibblem,byte). "
            "If omitted, uses the STAGE_ALGOS env var or all variations by default."
        ),
    )
    args = parser.parse_args()

    env_list = os.environ.get("STAGE_ALGOS")
    if args.algos:
        algos = [a.strip() for a in args.algos.split(",") if a.strip()]
    elif env_list:
        algos = [a.strip() for a in env_list.split(",") if a.strip()]
    else:
        algos = ["bit", "nibble", "nibblem", "byte"]

    rc = stage_variations(algos)
    if rc != 0:
        print("Some copy operations failed or had missing sources.")
        sys.exit(rc)


if __name__ == "__main__":
    main()
