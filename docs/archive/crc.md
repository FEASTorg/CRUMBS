# CRC Variants

CRC Variant Selection for CRUMBS Protocol (ATmega328P Target).

| Variant        | Flash (bytes) | RAM (bytes) | Speed (μs/kiB) | Suitability                             |
| -------------- | ------------- | ----------- | -------------- | --------------------------------------- |
| `crc8_bit`     | ~80           | 0           | 7800–18000     | Too slow                                |
| `crc8_nibble`  | ~130          | 0           | 5300–7600      | _**Best balance**_                      |
| `crc8_nibblem` | ~130          | 16          | 4900–7200      | Slightly faster, costs RAM              |
| `crc8_byte`    | ~300          | 0           | 900–2200       | Fastest, wasteful on flash-limited MCUs |

**Choice:** `crc8_nibble` (use `crc8_nibble_calculate()` API)  
**Reason:** Optimal tradeoff between flash size, speed, and zero RAM use for AVR-class MCUs (e.g., ATmega328P).  
**CRC field:** 1 byte, calculated over first 31 bytes of message payload.

## CRC Code Generation

The CRC generation workflow has been simplified and consolidated into a single script:

- `scripts/generate_crc8.py` — generates C99 outputs via pycrc and optionally stages variants into `src/crc` so the library compiles against a chosen CRC-8 implementation.

What it produces:

- Generated C99 outputs (per-variant):
  - `dist/crc/c99/crc8_<variant>.h`
  - `dist/crc/c99/crc8_<variant>.c`

- Staged (copied) files (if staging enabled):
  - `src/crc/crc8_<variant>.h`
  - `src/crc/crc8_<variant>.c`

Usage examples and notes:

    # Generate the default variant (nibble) and stage it into src/crc
    python scripts/generate_crc8.py

    # Generate nibble+byte variants and *don't* stage them into src/ (writes only to dist/)
    python scripts/generate_crc8.py --algos nibble,byte --no-stage

    # Generate all variants and stage them (bit,nibble,nibblem,byte)
    python scripts/generate_crc8.py --algos bit,nibble,nibblem,byte

By default the script stages `nibble` (the variant chosen by the project) to keep the
repository in a consistent state. The generator is driven by `pycrc` and produces
portable, C99-compatible implementations.

If you previously relied on separate conversion or staging scripts (`generate_crc8_c99.py`,
`validate_and_stage_crc8.py`, `generate_crc8_arduino.py`) note that this new script replaces the
common C99 generation + staging flow. The Arduino conversion script remains available for
specialized Arduino-friendly conversions but is no longer part of the default generation +
staging workflow.

Refer to `scripts/generate_crc8.py --help` for full options and usage.
