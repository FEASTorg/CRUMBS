# Message Helpers

The `crumbs_message_helpers.h` header provides zero-overhead inline helpers for building and reading message payloads. These helpers eliminate manual byte manipulation when working with multi-byte integers and floats.

## Overview

Instead of manually encoding values into `msg.data[]`:

```c
// Manual approach (error-prone)
msg.data[0] = angle & 0xFF;
msg.data[1] = (angle >> 8) & 0xFF;
msg.data_len = 2;
```

Use the message helpers:

```c
// With helpers (type-safe, bounds-checked)
#include "crumbs_message_helpers.h"

crumbs_msg_init(&msg, type_id, opcode);
crumbs_msg_add_u16(&msg, angle);
```

## Including the Header

```c
#include "crumbs_message_helpers.h"    // Linux/CMake projects
#include <crumbs_message_helpers.h>    // Arduino projects
```

The header is standalone and only depends on `<stdint.h>`, `<stddef.h>`, and `<string.h>`.

## Building Messages

### Initialization

Always initialize a message before adding data:

```c
crumbs_message_t msg;
crumbs_msg_init(&msg, MY_TYPE_ID, MY_COMMAND);
```

`crumbs_msg_init()` zeros the message and sets `type_id`, `opcode`, with `data_len = 0`.

### Adding Values

All add functions return 0 on success, -1 if the value would overflow the 27-byte payload limit.

```c
// Unsigned integers (little-endian encoding)
crumbs_msg_add_u8(&msg, value);     // 1 byte
crumbs_msg_add_u16(&msg, value);    // 2 bytes
crumbs_msg_add_u32(&msg, value);    // 4 bytes

// Signed integers (little-endian encoding)
crumbs_msg_add_i8(&msg, value);     // 1 byte
crumbs_msg_add_i16(&msg, value);    // 2 bytes
crumbs_msg_add_i32(&msg, value);    // 4 bytes

// Floating point (native byte order - see notes)
crumbs_msg_add_float(&msg, value);  // 4 bytes

// Raw bytes
crumbs_msg_add_bytes(&msg, ptr, len);
```

### Example: Building a Servo Command

```c
crumbs_message_t msg;
crumbs_msg_init(&msg, 0x02, 0x02);  // type=Servo, opcode=SetBoth

crumbs_msg_add_u16(&msg, 1500);  // Servo 1: 1500μs
crumbs_msg_add_u16(&msg, 2000);  // Servo 2: 2000μs
// msg.data_len is now 4
```

## Reading Payloads

### Basic Pattern

Use explicit byte offsets to track position while reading:

```c
void my_handler(crumbs_context_t *ctx, uint8_t cmd,
                const uint8_t *data, uint8_t len, void *user) {
    uint16_t servo1, servo2;

    if (crumbs_msg_read_u16(data, len, 0, &servo1) < 0) return;
    if (crumbs_msg_read_u16(data, len, 2, &servo2) < 0) return;

    // Use servo1, servo2...
}
```

### Reading Functions

All read functions return 0 on success, -1 if reading would exceed the buffer bounds.

```c
// Unsigned integers
crumbs_msg_read_u8(data, len, offset, &out);
crumbs_msg_read_u16(data, len, offset, &out);
crumbs_msg_read_u32(data, len, offset, &out);

// Signed integers
crumbs_msg_read_i8(data, len, offset, &out);
crumbs_msg_read_i16(data, len, offset, &out);
crumbs_msg_read_i32(data, len, offset, &out);

// Floating point
crumbs_msg_read_float(data, len, offset, &out);

// Raw bytes
crumbs_msg_read_bytes(data, len, offset, dest, count);
```

### Example: Reading LED Command

```c
void handle_led_set_one(crumbs_context_t *ctx, uint8_t cmd,
                        const uint8_t *data, uint8_t len, void *user) {
    uint8_t led_index, r, g, b;

    if (crumbs_msg_read_u8(data, len, 0, &led_index) < 0) return;
    if (crumbs_msg_read_u8(data, len, 1, &r) < 0) return;
    if (crumbs_msg_read_u8(data, len, 2, &g) < 0) return;
    if (crumbs_msg_read_u8(data, len, 3, &b) < 0) return;

    set_led(led_index, r, g, b);
}
```

## Creating Command Headers

The recommended pattern is to create a command header file for each device type. See `examples/handlers_usage/mock_ops.h` for a complete example.

### Pattern

```c
// my_device_commands.h
#ifndef MY_DEVICE_COMMANDS_H
#define MY_DEVICE_COMMANDS_H

#include "crumbs.h"
#include "crumbs_message_helpers.h"

#define MY_DEVICE_TYPE_ID   0x10

// Command types
#define MY_CMD_ACTION_A     0x01
#define MY_CMD_ACTION_B     0x02

// Sender functions
static inline int my_send_action_a(crumbs_context_t *ctx, uint8_t addr,
                                   uint16_t param1, uint8_t param2,
                                   crumbs_i2c_write_fn write_fn, void *write_ctx) {
    crumbs_message_t msg;
    crumbs_msg_init(&msg, MY_DEVICE_TYPE_ID, MY_CMD_ACTION_A);

    crumbs_msg_add_u16(&msg, param1);
    crumbs_msg_add_u8(&msg, param2);

    return crumbs_controller_send(ctx, addr, &msg, write_fn, write_ctx);
}

#endif
```

### Benefits of Command Headers

1. **Type-safe**: Function parameters enforce correct types
2. **Self-documenting**: Command names and parameters are explicit
3. **Reusable**: Same header works on controller and peripheral
4. **Composable**: Include multiple device headers in one controller

## Notes on Encoding

### Integer Encoding

All multi-byte integers use **little-endian** encoding:

- LSB first, MSB last
- Matches x86, ARM, and most microcontrollers
- Consistent across platforms

### Float Encoding

Floats use **native byte order** (memcpy):

- Works correctly when both endpoints have the same endianness
- Most embedded systems are little-endian (AVR, ARM Cortex-M, ESP32)
- **Caution**: If crossing to a big-endian system, floats may be misinterpreted

For maximum portability, consider encoding floats as fixed-point integers:

```c
// Instead of float
crumbs_msg_add_float(&msg, 25.5f);

// Use fixed-point (e.g., tenths of a degree)
crumbs_msg_add_i16(&msg, 255);  // 25.5 × 10
```

## API Reference

### Message Builder

| Function                          | Description                     | Bytes |
| --------------------------------- | ------------------------------- | ----- |
| `crumbs_msg_init(msg, type, op)`  | Zero message, set type+opcode   | —     |
| `crumbs_msg_add_u8(msg, v)`       | Append uint8_t                  | 1     |
| `crumbs_msg_add_u16(msg, v)`      | Append uint16_t (little-endian) | 2     |
| `crumbs_msg_add_u32(msg, v)`      | Append uint32_t (little-endian) | 4     |
| `crumbs_msg_add_i8(msg, v)`       | Append int8_t                   | 1     |
| `crumbs_msg_add_i16(msg, v)`      | Append int16_t (little-endian)  | 2     |
| `crumbs_msg_add_i32(msg, v)`      | Append int32_t (little-endian)  | 4     |
| `crumbs_msg_add_float(msg, v)`    | Append float (native order)     | 4     |
| `crumbs_msg_add_bytes(msg, p, n)` | Append n bytes from p           | n     |

### Message Reader

| Function                                      | Description                   | Bytes |
| --------------------------------------------- | ----------------------------- | ----- |
| `crumbs_msg_read_u8(d, len, off, &out)`       | Read uint8_t at offset        | 1     |
| `crumbs_msg_read_u16(d, len, off, &out)`      | Read uint16_t (little-endian) | 2     |
| `crumbs_msg_read_u32(d, len, off, &out)`      | Read uint32_t (little-endian) | 4     |
| `crumbs_msg_read_i8(d, len, off, &out)`       | Read int8_t                   | 1     |
| `crumbs_msg_read_i16(d, len, off, &out)`      | Read int16_t (little-endian)  | 2     |
| `crumbs_msg_read_i32(d, len, off, &out)`      | Read int32_t (little-endian)  | 4     |
| `crumbs_msg_read_float(d, len, off, &out)`    | Read float (native order)     | 4     |
| `crumbs_msg_read_bytes(d, len, off, dest, n)` | Read n bytes into dest        | n     |

All functions return 0 on success, -1 on bounds error.

## See Also

- [API Reference](api-reference.md) — Core CRUMBS API
- [Examples](examples.md) — Complete usage examples
- [Protocol](protocol.md) — Wire format specification
