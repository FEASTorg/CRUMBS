# Calculator Peripheral

Simple 32-bit integer calculator peripheral demonstrating the function-style interface pattern.

## Hardware

- **Board:** Arduino Nano (ATmega328P)
- **I2C Address:** 0x10
- **I2C Pins:** A4 (SDA), A5 (SCL)
- **Type ID:** 0x03

## Features

- **Arithmetic Operations:** ADD, SUB, MUL, DIV on 32-bit unsigned integers
- **Result Storage:** Last result retrievable via GET_RESULT
- **History Buffer:** Circular buffer storing last 12 operations
- **Pure Software:** No additional hardware required

## Building

### PlatformIO

```bash
cd calculator
pio run                    # Build
pio run -t upload          # Upload to Arduino Nano
pio device monitor         # View serial output
```

## Operations

### SET Commands (Execute Calculations)

| Opcode | Command | Payload          | Description                             |
| ------ | ------- | ---------------- | --------------------------------------- |
| 0x01   | ADD     | `[a:u32][b:u32]` | Add a + b                               |
| 0x02   | SUB     | `[a:u32][b:u32]` | Subtract a - b                          |
| 0x03   | MUL     | `[a:u32][b:u32]` | Multiply a \* b                         |
| 0x04   | DIV     | `[a:u32][b:u32]` | Divide a / b (div by zero = 0xFFFFFFFF) |

### GET Commands (Query Results via SET_REPLY)

| Opcode    | Command        | Reply                              | Description                       |
| --------- | -------------- | ---------------------------------- | --------------------------------- |
| 0x80      | GET_RESULT     | `[result:u32]`                     | Last calculation result           |
| 0x81      | GET_HIST_META  | `[count:u8][write_pos:u8]`         | History metadata                  |
| 0x82–0x8D | GET_HIST_0..11 | `[op:4][a:u32][b:u32][result:u32]` | Specific history entry (16 bytes) |

## History Entry Format

Each history entry is 16 bytes:

- **Bytes 0-3:** Operation name ("ADD\0", "SUB\0", "MUL\0", "DIV\0")
- **Bytes 4-7:** First operand (u32, little-endian)
- **Bytes 8–11:** Second operand (u32, little-endian)
- **Bytes 12–15:** Result (u32, little-endian)

## Example Usage

From a Linux controller:

```c
#include "calculator_ops.h"

/* Execute calculation */
calc_send_add(&ctx, 0x10, write_fn, io, 123, 456);  // 123 + 456

/* Retrieve result */
uint32_t result;
calc_get_result(&ctx, 0x10, write_fn, read_fn, io, &result);
printf("Result: %u\n", result);  // 579

/* Check history */
uint8_t count, write_pos;
calc_get_hist_meta(&ctx, 0x10, write_fn, read_fn, io, &count, &write_pos);
printf("History: %u entries, next write at %u\n", count, write_pos);

/* Retrieve history entry */
uint8_t entry[16];
calc_get_hist_entry(&ctx, 0x10, write_fn, read_fn, io, 0, entry);
// Parse entry...
```

## Serial Output

The peripheral logs all operations to serial:

```text
=== CRUMBS Calculator Peripheral ===
I2C Address: 0x10
Type ID: 0x03
Operations: ADD, SUB, MUL, DIV
History: 12 entries (circular buffer)

Calculator peripheral ready!
Waiting for I2C commands...

ADD: 123 + 456 = 579
MUL: 10 * 20 = 200
DIV: 100 / 5 = 20
```

## Notes

- All integers are **unsigned 32-bit** (0 to 4,294,967,295)
- Division by zero returns **0xFFFFFFFF**
- History buffer is **circular** (oldest entries overwritten when full)
- All multi-byte values use **little-endian** encoding
