#!/usr/bin/env python3
import argparse
import subprocess
from pathlib import Path
from datetime import datetime
import sys
import re

CONVERT_TO_PROGMEM = True
PRESERVE_C_FILES = False

# -------------------------------------------------------------
# Helpers
# -------------------------------------------------------------


def run_pycrc(model: str, flags: str, output: Path, mode: str) -> None:
    """Call pycrc to generate .h or .c."""
    cmd = [
        sys.executable,
        "-m",
        "pycrc",
        "--model",
        model,
        *flags.split(),
        "--generate",
        mode,
        "-o",
        str(output),
    ]
    subprocess.run(cmd, check=True)


def autogen_comment_lines() -> list[str]:
    """Lines inserted by the original ed script (text identical, date differs)."""
    when = datetime.now().strftime("%c")
    # Match original style as closely as possible; only the timestamp content differs.
    return [
        f" * Auto converted to Arduino C++ on {when}",
        " * by AceCRC (https://github.com/bxparks/AceCRC).",
        " * DO NOT EDIT",
    ]


def insert_before_first_match(
    lines: list[str], predicate, new_lines: list[str]
) -> list[str]:
    for idx, line in enumerate(lines):
        if predicate(line):
            return lines[:idx] + new_lines + lines[idx:]
    # If not found, append at end (should not happen with pycrc output)
    return lines + new_lines


def replace_range_with(
    lines: list[str], start_idx: int, count: int, new_lines: list[str]
) -> list[str]:
    return lines[:start_idx] + new_lines + lines[start_idx + count :]


# -------------------------------------------------------------
# Header Conversion (.h → .hpp) – faithful to generate.sh
# -------------------------------------------------------------


def convert_h_to_hpp(
    modelroot: str, algotag: str, model: str, old_guard: str, new_guard: str
) -> None:
    fileroot = f"{modelroot}_{algotag}"
    hfile = Path(f"{fileroot}.h")
    out = Path(f"{fileroot}.hpp")

    lines = hfile.read_text().splitlines()

    # 1) Insert auto-generated comment before the line matching " */"
    auto_lines = autogen_comment_lines()
    lines = insert_before_first_match(
        lines,
        lambda l: " */" in l,
        auto_lines,
    )

    # 2) Replace header guard in #ifndef and the next line (#define)
    guard_start_idx = None
    for i, line in enumerate(lines):
        if f"#ifndef {old_guard}" in line:
            guard_start_idx = i
            break
    if guard_start_idx is not None and guard_start_idx + 1 < len(lines):
        for j in (guard_start_idx, guard_start_idx + 1):
            lines[j] = lines[j].replace(old_guard, new_guard)

    # 3) First #ifdef __cplusplus block → namespace open
    #    .,+2c   namespace ace_crc { namespace fileroot {
    first_cpp_idx = None
    for i, line in enumerate(lines):
        if line.strip().startswith("#ifdef __cplusplus"):
            first_cpp_idx = i
            break
    if first_cpp_idx is not None:
        ns_open_block = [
            f"namespace ace_crc {{",
            f"namespace {fileroot} {{",
        ]
        # Replace this line and next 2 lines
        lines = replace_range_with(lines, first_cpp_idx, 3, ns_open_block)

    # 4) Convert #define CRC_ALGO_* line to "static const uint8_t ... = N;"
    for i, line in enumerate(lines):
        if "CRC_ALGO" in line and line.strip().startswith("#define"):
            # naive but safe given pycrc's formatting
            parts = line.split()
            # Expect: #define NAME VALUE
            if len(parts) >= 3:
                name = parts[1]
                value = parts[2]
                lines[i] = f"static const uint8_t {name} = {value};"
            break

    # 5) Second #ifdef __cplusplus block → oneshot wrapper + namespace close
    #    .,+2c  big block (wrapper + "} // fileroot" + "} // ace_crc")
    second_cpp_idx = None
    for i, line in enumerate(lines):
        if line.strip().startswith("#ifdef __cplusplus"):
            second_cpp_idx = i
            break
    if second_cpp_idx is not None:
        oneshot_block = [
            "/**",
            " * Calculate the crc in one-shot.",
            " * This is a convenience function added by AceCRC.",
            " *",
            " * \\param[in] data     Pointer to a buffer of \\a data_len bytes.",
            " * \\param[in] data_len Number of bytes in the \\a data buffer.",
            " */",
            "inline crc_t crc_calculate(const void *data, size_t data_len) {",
            "  crc_t crc = crc_init();",
            "  crc = crc_update(crc, data, data_len);",
            "  return crc_finalize(crc);",
            "}",
            "",
            f"}} // {fileroot}",
            "} // ace_crc",
        ]
        lines = replace_range_with(lines, second_cpp_idx, 3, oneshot_block)

    # 6) Final #endif line that references old_header_guard → substitute new_guard there, too
    for i, line in enumerate(lines):
        if "#endif" in line and old_guard in line:
            lines[i] = line.replace(old_guard, new_guard)
            break

    # 7) Remove leading "static " at start of lines (1,$s/^static //)
    for i, line in enumerate(lines):
        if line.lstrip().startswith("static "):
            prefix_len = len(line) - len(line.lstrip())
            indent = line[:prefix_len]
            body = line[prefix_len:]
            if body.startswith("static "):
                body = body[len("static ") :]
            lines[i] = indent + body

    # 8) /typedef uint_fast/s//typedef uint/
    for i, line in enumerate(lines):
        if "typedef uint_fast" in line:
            lines[i] = line.replace("typedef uint_fast", "typedef uint", 1)
            break

    out.write_text("\n".join(lines) + "\n")

    if not PRESERVE_C_FILES:
        hfile.unlink()


# -------------------------------------------------------------
# Source Conversion (.c → .cpp) – faithful to generate.sh
# -------------------------------------------------------------


def convert_c_to_cpp(
    modelroot: str, algotag: str, model: str, pgm_read_func: str
) -> None:
    fileroot = f"{modelroot}_{algotag}"
    cfile = Path(f"{fileroot}.c")
    out = Path(f"{fileroot}.cpp")

    lines = cfile.read_text().splitlines()

    # Common step: insert auto comment before " */"
    auto_lines = autogen_comment_lines()
    lines = insert_before_first_match(
        lines,
        lambda l: " */" in l,
        auto_lines,
    )

    # Replace include "fileroot.h" → .hpp with original comment
    include_pattern = f'#include "{fileroot}.h"'
    for i, line in enumerate(lines):
        if include_pattern in line:
            lines[i] = f'#include "{fileroot}.hpp" // header file converted by AceCRC'
            include_idx = i
            break
    else:
        include_idx = None

    # Determine loop var name
    if algotag == "bit":
        loop_var = "i"
    else:
        loop_var = "tbl_idx"

    no_progmem = (algotag in ("bit", "nibblem")) or (not CONVERT_TO_PROGMEM)

    if no_progmem:
        # BIT / NIBBLEM / CONVERT_TO_PROGMEM==0 path

        # After first empty line, append namespace open block
        inserted_ns = False
        for i, line in enumerate(lines):
            if line.strip() == "":
                ns_block = [
                    "namespace ace_crc {",
                    f"namespace {fileroot} {{",
                    "",
                ]
                lines = lines[: i + 1] + ns_block + lines[i + 1 :]
                inserted_ns = True
                break
        if not inserted_ns:
            ns_block = [
                "",
                "namespace ace_crc {",
                f"namespace {fileroot} {{",
                "",
            ]
            lines += ns_block

        # Move current position to /crc_update/ then change /unsigned int loop_var;/
        # We just search globally for that line; safe given pycrc output
        for i, line in enumerate(lines):
            if f"unsigned int {loop_var};" in line:
                lines[i] = f"    uint8_t {loop_var};"
                break

        # Append namespace close at EOF
        lines.append("")
        lines.append(f"}} // {fileroot}")
        lines.append("} // ace_crc")

    else:
        # PROGMEM path (nibble/byte when CONVERT_TO_PROGMEM==1)

        # After changing include, insert pgmspace include block before that line
        if include_idx is not None:
            pgm_block = [
                "#if defined(ARDUINO_ARCH_AVR) || defined(ARDUINO_ARCH_SAMD)",
                "  #include <avr/pgmspace.h>",
                "#else",
                "  #include <pgmspace.h>",
                "#endif",
            ]
            lines = lines[:include_idx] + pgm_block + lines[include_idx:]

        # After first empty line, append namespace open block
        inserted_ns = False
        for i, line in enumerate(lines):
            if line.strip() == "":
                ns_block = [
                    "namespace ace_crc {",
                    f"namespace {fileroot} {{",
                    "",
                ]
                lines = lines[: i + 1] + ns_block + lines[i + 1 :]
                inserted_ns = True
                break
        if not inserted_ns:
            ns_block = [
                "",
                "namespace ace_crc {",
                f"namespace {fileroot} {{",
                "",
            ]
            lines += ns_block

        # /crc_table/s/= {/PROGMEM &/
        for i, line in enumerate(lines):
            if "crc_table" in line and "= {" in line:
                lines[i] = line.replace("= {", "PROGMEM = {", 1)
                break

        # g/crc_table\[\(tbl_idx.*\)\]/s//(pgm_read_x(crc_table + (\1)))/
        # Rough equivalent: replace "crc_table[EXPR]" with "(pgm_read_x(crc_table + (EXPR)))"
        pattern = re.compile(r"crc_table\[(tbl_idx[^\]]*)\]")
        for i, line in enumerate(lines):
            if "crc_table[" in line:
                lines[i] = pattern.sub(
                    lambda m: f"({pgm_read_func}(crc_table + ({m.group(1)})))", line
                )

        # /crc_update/ then change /unsigned int loop_var;/
        for i, line in enumerate(lines):
            if f"unsigned int {loop_var};" in line:
                lines[i] = f"    uint8_t {loop_var};"
                break

        # Append namespace close at EOF
        lines.append("")
        lines.append(f"}} // {fileroot}")
        lines.append("} // ace_crc")

    out.write_text("\n".join(lines) + "\n")

    if not PRESERVE_C_FILES:
        cfile.unlink()


# -------------------------------------------------------------
# CLI + Orchestration
# -------------------------------------------------------------


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--model", required=True)
    parser.add_argument(
        "--algotag",
        required=True,
        choices=("bit", "nibble", "nibblem", "byte"),
    )
    parser.add_argument("--header", action="store_true")
    parser.add_argument("--source", action="store_true")
    args = parser.parse_args()

    model = args.model
    algotag = args.algotag

    # Normalize model names (crc-16-ccitt → crc16ccitt)
    modelroot = model.replace("-", "")
    modelroot_upper = modelroot.upper()
    algotag_upper = algotag.upper()

    old_guard = f"{modelroot_upper}_{algotag_upper}_H"
    new_guard = f"ACE_CRC_{modelroot_upper}_{algotag_upper}_HPP"

    # pycrc flags
    algo_flags = {
        "bit": "--algorithm bit-by-bit-fast",
        "nibble": "--algorithm table-driven --table-idx-width 4",
        "nibblem": "--algorithm table-driven --table-idx-width 4",
        "byte": "--algorithm table-driven --table-idx-width 8",
    }[algotag]

    # pgm_read_x function
    pgm_map = {
        "crc-8": "pgm_read_byte",
        "crc-16-ccitt": "pgm_read_word",
        "crc-16-modbus": "pgm_read_word",
        "crc-32": "pgm_read_dword",
    }
    pgm_read_func = pgm_map.get(model)
    if pgm_read_func is None:
        raise ValueError(f"Unknown model {model}")

    fileroot = f"{modelroot}_{algotag}"

    if args.header:
        run_pycrc(model, algo_flags, Path(f"{fileroot}.h"), "h")
        convert_h_to_hpp(modelroot, algotag, model, old_guard, new_guard)

    if args.source:
        run_pycrc(model, algo_flags, Path(f"{fileroot}.c"), "c")
        convert_c_to_cpp(modelroot, algotag, model, pgm_read_func)


if __name__ == "__main__":
    main()
