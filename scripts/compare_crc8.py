#!/usr/bin/env python3
import difflib
from pathlib import Path
import argparse

SCRIPT_DIR = Path(__file__).parent
REF = SCRIPT_DIR / "ace_crc8"
OUT = SCRIPT_DIR / "arduino_crc8"
REPORT = SCRIPT_DIR / "comparison_crc8"

IGNORE_PATTERNS = [
    "Generated on",
    "by pycrc v",
    "Auto converted to",
    "DO NOT EDIT",
]


def filter_lines(lines):
    """Remove metadata lines, timestamps, and ignorable comment-only blanks."""
    result = []
    for ln in lines:
        stripped = ln.strip()

        # Ignore timestamps, DO NOT EDIT, pycrc version, etc.
        if any(p in ln for p in IGNORE_PATTERNS):
            continue

        # Ignore pure comment spacers like " *", "*", "/*", "*/"
        if stripped in ("*", "/*", "*/"):
            continue
        if stripped.startswith("*") and stripped[1:].strip() == "":
            continue

        # Ignore blank lines
        if stripped == "":
            continue

        result.append(ln.rstrip())

    return result


def compare_pair(ref: Path, gen: Path, strict: bool):
    """Return unified diff. If strict=False, filter superficial differences."""
    if strict:
        ref_lines = ref.read_text().splitlines()
        gen_lines = gen.read_text().splitlines()
    else:
        ref_lines = filter_lines(ref.read_text().splitlines())
        gen_lines = filter_lines(gen.read_text().splitlines())

    return list(
        difflib.unified_diff(
            ref_lines, gen_lines, fromfile=str(ref), tofile=str(gen), lineterm=""
        )
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--strict",
        action="store_true",
        help="Show full real diff without filtering",
    )
    args = parser.parse_args()

    strict = args.strict

    REPORT.mkdir(exist_ok=True)

    candidates = sorted(
        [f for f in OUT.iterdir() if f.is_file() and f.name.startswith("crc8")]
    )

    for gen_file in candidates:
        ref_file = REF / gen_file.name
        report_path = REPORT / (gen_file.name + ".diff")

        if not ref_file.exists():
            report_path.write_text(f"[MISSING] Reference file not found: {ref_file}\n")
            print(f"[MISSING] {gen_file.name}")
            continue

        diff = compare_pair(ref_file, gen_file, strict)

        if diff:
            report_path.write_text("\n".join(diff) + "\n")
            print(f"[DIFF] {gen_file.name}")
        else:
            print(f"[OK] {gen_file.name}")
            if report_path.exists():
                report_path.unlink()

    print("\nCRC-8 comparison complete.")


if __name__ == "__main__":
    main()
