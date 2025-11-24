#!/usr/bin/env python3
"""Generate Arduino-friendly CRC-8 C++ from the C99 pycrc outputs.

Unlike the AceCRC variant, this generator produces a cleaner single-level
namespace per algorithm (e.g. `crc8_nibble`) and avoids any reference to
ace_crc or nested wrapping; instead we generate a simple per-algo namespace.

This script expects the pycrc-generated C99 outputs to live in
    <repo-root>/dist/crc/c99
and will write Arduino-converted files to
    <repo-root>/dist/crc/arduino
"""

from pathlib import Path
from datetime import datetime
import re

SCRIPT_DIR = Path(__file__).parent
REPO_ROOT = SCRIPT_DIR.parent
C99_DIR = REPO_ROOT / "dist" / "crc" / "c99"
ARD_DIR = REPO_ROOT / "dist" / "crc" / "arduino"
ARD_DIR.mkdir(parents=True, exist_ok=True)

CONVERT_TO_PROGMEM = True


def autogen_comment():
    ts = datetime.now().strftime("%c")
    return [
        f" * Auto converted to Arduino C++ on {ts}",
        " * DO NOT EDIT",
    ]


def insert_auto_comment(lines):
    result = []
    inserted = False
    for line in lines:
        if (not inserted) and "*/" in line:
            result.extend(autogen_comment())
            inserted = True
        result.append(line)
    return result


def convert_header(hpath: Path, fileroot: str):
    out = ARD_DIR / (fileroot + ".hpp")

    old_guard = hpath.stem.upper() + "_H"
    # keep header guard readable and unique: e.g. CRC8_NIBBLE_HPP
    new_guard = fileroot.upper() + "_HPP"

    lines = hpath.read_text().splitlines()

    # Insert auto comment
    lines = insert_auto_comment(lines)

    # Fix header guards
    new = []
    for ln in lines:
        new.append(ln.replace(old_guard, new_guard))
    lines = new

    # Ensure the original extern "C" open is present.
    # pycrc C99 headers usually have the three-line pattern:
    #   #ifdef __cplusplus
    #   extern "C" {
    #   #endif
    # When converting, make sure the opening block is present so the
    # corresponding closing block at the file end remains matched.
    for i, ln in enumerate(lines):
        if ln.strip().startswith("#ifdef __cplusplus"):
            lines[i : i + 3] = [
                "#ifdef __cplusplus",
                'extern "C" {',
                "#endif",
            ]
            break

    # Convert CRC_ALGO define
    for i, ln in enumerate(lines):
        if ln.strip().startswith("#define CRC_ALGO"):
            parts = ln.split()
            name = parts[1]
            val = parts[2]
            lines[i] = f"static const uint8_t {name} = {val};"
            break

    # Remove leading "static " from top-level declarations (match ace_crc behavior)
    for i, ln in enumerate(lines):
        if ln.lstrip().startswith("static "):
            # preserve indentation
            prefix_len = len(ln) - len(ln.lstrip())
            lines[i] = ln[:prefix_len] + ln.lstrip()[len("static ") :]

    # Append a convenient oneshot function and keep it at global scope.
    # Insert it just before the final #ifdef __cplusplus (the closing
    # extern "C" block) if present, otherwise append at EOF.
    insert_at = None
    for i, ln in enumerate(lines):
        if ln.strip().startswith("#ifdef __cplusplus"):
            insert_at = i
    if insert_at is None:
        insert_at = len(lines)

    block = [
                "/**",
                " * Calculate the crc in one-shot.",
        " * Convenience helper exposed in the global namespace.",
                " *",
                " * \\param[in] data     Pointer to a buffer of \\a data_len bytes.",
                " * \\param[in] data_len Number of bytes in the \\a data buffer.",
                " */",
                "inline crc_t crc_calculate(const void *data, size_t data_len) {",
                "  crc_t crc = crc_init();",
                "  crc = crc_update(crc, data, data_len);",
                "  return crc_finalize(crc);",
                "}",
            ]
    # perform the insertion
    lines[insert_at:insert_at] = block

    # typedef uint_fast → typedef uint (keep pycrc compatibility)
    for i, ln in enumerate(lines):
        if "typedef uint_fast" in ln:
            lines[i] = ln.replace("typedef uint_fast", "typedef uint")

    # Prefix public symbols to avoid collisions when multiple variants are
    # included together. We add a fileroot-based prefix for types, functions
    # and table names. Example: crc_init -> crc8_nibble_init, crc_t -> crc8_nibble_t
    prefix = fileroot  # e.g. 'crc8_nibble'

    def wreplace(s: str) -> str:
        # word-boundary replacements for common identifiers
        s = re.sub(r"\bcrc_t\b", f"{prefix}_t", s)
        s = re.sub(r"\bcrc_init\b", f"{prefix}_init", s)
        s = re.sub(r"\bcrc_update\b", f"{prefix}_update", s)
        s = re.sub(r"\bcrc_finalize\b", f"{prefix}_finalize", s)
        s = re.sub(r"\bcrc_calculate\b", f"{prefix}_calculate", s)
        s = re.sub(r"\bcrc_table\b", f"{prefix}_crc_table", s)
        s = re.sub(r"\bCRC_ALGO_([A-Z0-9_]+)\b", lambda m: f"{prefix.upper()}_ALGO_{m.group(1)}", s)
        return s

    lines = [wreplace(ln) for ln in lines]

    out.write_text("\n".join(lines) + "\n")


def convert_source(cpath: Path, fileroot: str, pgm_read: str):
    out = ARD_DIR / (fileroot + ".cpp")
    lines = cpath.read_text().splitlines()

    lines = insert_auto_comment(lines)

    # Replace header include — replace the whole line so we don't leave
    # trailing pycrc comments behind
    old = f'#include "{fileroot}.h"'
    new = f'#include "{fileroot}.hpp" // header file converted by generator'
    for i, ln in enumerate(lines):
        if old in ln:
            lines[i] = new
            break

    algotag = fileroot.split("_")[1]
    loop_var = "i" if algotag == "bit" else "tbl_idx"
    use_progmem = CONVERT_TO_PROGMEM and algotag in ("nibble", "byte")

    # Insert PROGMEM includes
    if use_progmem:
        for i, ln in enumerate(lines):
            if ln.startswith("#include"):
                block = [
                    "#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAMD)",
                    "  #include <avr/pgmspace.h>",
                    "#else",
                    "  #include <pgmspace.h>",
                    "#endif",
                ]
                lines = lines[:i] + block + lines[i:]
                break

    # Don't insert any namespace wrappers; keep implementation in global scope

    # Loop var replacement
    for i, ln in enumerate(lines):
        if f"unsigned int {loop_var};" in ln:
            lines[i] = f"    uint8_t {loop_var};"
            break

    # PROGMEM table conversion
    if use_progmem:
        for i, ln in enumerate(lines):
            if "crc_table" in ln and "= {" in ln:
                lines[i] = ln.replace("= {", "PROGMEM = {")
                break

        pattern = re.compile(r"crc_table\[(tbl_idx[^\]]*)\]")
        for i, ln in enumerate(lines):
            if "crc_table[" in ln:
                lines[i] = pattern.sub(
                    lambda m: f"({pgm_read}(crc_table + ({m.group(1)})))", ln
                )

    # After making source edits, apply the same prefix replacements
    # used in the header conversion so implementation symbols match.
    prefix = fileroot

    def wreplace(s: str) -> str:
        s = re.sub(r"\bcrc_t\b", f"{prefix}_t", s)
        s = re.sub(r"\bcrc_init\b", f"{prefix}_init", s)
        s = re.sub(r"\bcrc_update\b", f"{prefix}_update", s)
        s = re.sub(r"\bcrc_finalize\b", f"{prefix}_finalize", s)
        s = re.sub(r"\bcrc_calculate\b", f"{prefix}_calculate", s)
        s = re.sub(r"\bcrc_table\b", f"{prefix}_crc_table", s)
        s = re.sub(r"\bCRC_ALGO_([A-Z0-9_]+)\b", lambda m: f"{prefix.upper()}_ALGO_{m.group(1)}", s)
        return s

    lines = [wreplace(ln) for ln in lines]

    out.write_text("\n".join(lines) + "\n")


def main():
    print("Converting C99 → Arduino C++ ...")

    for cfile in sorted(C99_DIR.glob("crc8_*.c")):
        algo = cfile.stem.split("_")[1]
        fileroot = f"crc8_{algo}"

        hfile = C99_DIR / (fileroot + ".h")
        if not hfile.exists():
            print("Missing header:", hfile)
            continue

        # CRC-8 always uses pgm_read_byte for lookup tables
        convert_header(hfile, fileroot)
        convert_source(cfile, fileroot, "pgm_read_byte")

    print("\nDone. Arduino CRC-8 files stored in:", ARD_DIR)


if __name__ == "__main__":
    main()
