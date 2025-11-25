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

The CRC code used in CRUMBS is generated using the following files found in the `scripts/` directory:

- `generate_crc8_c99.py`: Python script to generate C99-compatible CRC code using pycrc.
- `generate_crc8_arduino.py`: Python script to generate Arduino-compatible CRC code from the C99 code.
- `validate_and_stage_crc8.py`: Python script to validate the generated CRC code and move it to the appropriate `src/crc/` directory.

**Note**: The CRC implementations included in this repository are generated using [pycrc](https://github.com/tpircher/pycrc). The generation approach here is inspired by the [AceCRC](https://github.com/bxparks/AceCRC) project, but the library ships a simplified locally-named API (e.g., `crc8_nibble_*`) and does not require AceCRC as a runtime dependency.
