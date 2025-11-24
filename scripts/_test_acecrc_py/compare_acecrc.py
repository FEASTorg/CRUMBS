#!/usr/bin/env python3
import difflib
from pathlib import Path
import re

SCRIPT_DIR = Path(__file__).parent
REF_DIR = SCRIPT_DIR / "ace_crc"
OUT_DIR = SCRIPT_DIR / "output"
REPORT_DIR = SCRIPT_DIR / "comparison_reports"

# Ignore small/noise patterns
IGNORE_PATTERNS = [
    "Generated on",
    "by pycrc v",
    "Auto converted to",
    "DO NOT EDIT",
]

# Ignore lines that are:
# - empty
# - only whitespace
# - a lone " *"
BLANK_NOISE = re.compile(r"^\s*(\*?)\s*$")


def normalize_lines(lines):
    """Apply ignore rules and normalize whitespace noise."""
    cleaned = []
    for line in lines:
        # Case 1: ignore lines containing any ignore pattern
        if any(p in line for p in IGNORE_PATTERNS):
            continue

        # Case 2: ignore lines that are blank or noise comments
        if BLANK_NOISE.match(line):
            continue

        cleaned.append(line.rstrip())  # strip trailing whitespace for consistency
    return cleaned


def filtered_diff(ref: Path, out: Path):
    """Return unified diff of filtered content."""
    ref_lines = normalize_lines(ref.read_text().splitlines())
    out_lines = normalize_lines(out.read_text().splitlines())

    # If normalized content is identical, return empty
    if ref_lines == out_lines:
        return []

    # Return unified diff for the parts that are different
    return list(
        difflib.unified_diff(
            ref_lines,
            out_lines,
            fromfile=str(ref),
            tofile=str(out),
            lineterm="",
        )
    )


def main():
    REPORT_DIR.mkdir(exist_ok=True)

    all_files = sorted([f.name for f in OUT_DIR.iterdir() if f.is_file()])

    for filename in all_files:
        f_out = OUT_DIR / filename
        f_ref = REF_DIR / filename

        if not f_ref.exists():
            # We only write a report if reference is missing
            report_path = REPORT_DIR / f"{filename}.diff"
            report_path.write_text(f"[MISSING] Reference file not found: {f_ref}\n")
            print(f"[MISSING] {filename}")
            continue

        diff = filtered_diff(f_ref, f_out)

        if diff:
            report_path = REPORT_DIR / f"{filename}.diff"
            report_path.write_text("\n".join(diff) + "\n")
            print(f"[DIFF] {filename}")
        else:
            # No report if identical (cleaner)
            print(f"[OK] {filename}")

    print(f"\nDone. Only REAL diffs written to: {REPORT_DIR}")


if __name__ == "__main__":
    main()
