#!/usr/bin/env python3
import sys
from pathlib import Path
from datetime import datetime
import re

SCRIPT_DIR = Path(__file__).parent
C99_DIR = SCRIPT_DIR / "c99_crc8"
ARD_DIR = SCRIPT_DIR / "arduino_crc8"
ARD_DIR.mkdir(exist_ok=True)

CONVERT_TO_PROGMEM = True


def autogen_comment():
    ts = datetime.now().strftime("%c")
    return [
        f" * Auto converted to Arduino C++ on {ts}",
        " * by AceCRC (https://github.com/bxparks/AceCRC).",
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
    new_guard = "ACE_CRC_" + old_guard.replace("_H", "_HPP")

    lines = hpath.read_text().splitlines()

    # Insert auto comment
    lines = insert_auto_comment(lines)

    # Fix header guards
    new = []
    for ln in lines:
        new.append(ln.replace(old_guard, new_guard))
    lines = new

    # First __cplusplus → open namespace
    for i, ln in enumerate(lines):
        if ln.strip().startswith("#ifdef __cplusplus"):
            lines[i : i + 3] = [
                "namespace ace_crc {",
                f"namespace {fileroot} {{",
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

    # Second __cplusplus → oneshot and close
    for i, ln in enumerate(lines):
        if ln.strip().startswith("#ifdef __cplusplus"):
            block = [
                "/**",
                " * Calculate the crc in one-shot.",
                " * This is a convenience function added by AceCRC.",
                " *",
                " * \\param[in] data     Pointer to a buffer of \\a data_len bytes.",
                " * \\param[in] data_len Number of bytes in the \\a data buffer.",
                "inline crc_t crc_calculate(const void *data, size_t data_len) {",
                "  crc_t crc = crc_init();",
                "  crc = crc_update(crc, data, data_len);",
                "  return crc_finalize(crc);",
                "}",
                f"}} // {fileroot}",
                "} // ace_crc",
            ]
            lines[i : i + 3] = block
            break

    # typedef uint_fast → typedef uint
    for i, ln in enumerate(lines):
        if "typedef uint_fast" in ln:
            lines[i] = ln.replace("typedef uint_fast", "typedef uint")

    out.write_text("\n".join(lines) + "\n")


def convert_source(cpath: Path, fileroot: str, pgm_read: str):
    out = ARD_DIR / (fileroot + ".cpp")
    lines = cpath.read_text().splitlines()

    lines = insert_auto_comment(lines)

    # Replace header include — replace the whole line so we don't leave
    # trailing pycrc comments behind
    old = f'#include "{fileroot}.h"'
    new = f'#include "{fileroot}.hpp" // header file converted by AceCRC'
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

    # Insert namespace after first blank line
    for i, ln in enumerate(lines):
        if ln.strip() == "":
            ns_block = [
                "namespace ace_crc {",
                f"namespace {fileroot} {{",
                "",
            ]
            lines = lines[: i + 1] + ns_block + lines[i + 1 :]
            break

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

    # Close namespaces
    lines.append("")
    lines.append(f"}} // {fileroot}")
    lines.append("} // ace_crc")

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
