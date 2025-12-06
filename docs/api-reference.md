# API Reference

This file documents the public CRUMBS C API (core + provided HAL adapters) so you can use CRUMBS as a compiled library without reading the implementation.

The core is intentionally small and C-friendly so it is easy to consume from Arduino sketches, PlatformIO projects or native Linux programs.

## Key types and constants

- `crumbs_message_t` — message struct (see `src/crumbs_message.h`). Fields:

  - `address` — device address (NOT serialized)
  - `type_id` — 1 byte
  - `command_type` — 1 byte
  - `data_len` — payload length (0–27)
  - `data[27]` — raw byte payload (opaque to CRUMBS)
  - `crc8` — 1 byte CRC-8 computed over the serialized header + payload

- `crumbs_context_t` — per-endpoint context (role, address, CRC statistics, callbacks, user_data)
- `CRUMBS_MAX_PAYLOAD` — maximum payload size (27 bytes)
- `CRUMBS_MESSAGE_MAX_SIZE` — maximum serialized frame size (31 bytes)

Serialized frame layout (4–31 bytes):

- type_id (1)
- command_type (1)
- data_len (1)
- data[0..data_len-1] (variable, 0–27 bytes)
- crc8 (1)

CRC is computed over: type_id + command_type + data_len + data[0..data_len-1]

## Core API

```c
/* Initialize a context */
void crumbs_init(crumbs_context_t *ctx, crumbs_role_t role, uint8_t address);

/* Install callbacks */
void crumbs_set_callbacks(crumbs_context_t *ctx,
                          crumbs_message_cb_t on_message,
                          crumbs_request_cb_t on_request,
                          void *user_data);

/* Per-command handler registration */
int crumbs_register_handler(crumbs_context_t *ctx, uint8_t command_type, crumbs_handler_fn fn, void *user_data);
int crumbs_unregister_handler(crumbs_context_t *ctx, uint8_t command_type);

/* Encoding/decoding */
size_t crumbs_encode_message(const crumbs_message_t *msg, uint8_t *buffer, size_t buffer_len);
int crumbs_decode_message(const uint8_t *buffer, size_t buffer_len, crumbs_message_t *msg, crumbs_context_t *ctx);

/* Controller send adapter */
int crumbs_controller_send(const crumbs_context_t *ctx,
                           uint8_t target_addr,
                           const crumbs_message_t *msg,
                           crumbs_i2c_write_fn write_fn,
                           void *write_ctx);

/* Peripheral helpers */
int crumbs_peripheral_handle_receive(crumbs_context_t *ctx, const uint8_t *buffer, size_t len);
int crumbs_peripheral_build_reply(crumbs_context_t *ctx, uint8_t *out_buf, size_t out_buf_len, size_t *out_len);

/* CRC stats */
uint32_t crumbs_get_crc_error_count(const crumbs_context_t *ctx);
int crumbs_last_crc_ok(const crumbs_context_t *ctx);
void crumbs_reset_crc_stats(crumbs_context_t *ctx);

/* ABI compatibility check */
size_t crumbs_context_size(void);
```

### ABI Compatibility Check

The `crumbs_context_size()` function returns the size of `crumbs_context_t` as compiled into the library. This is used to detect mismatches between the header and compiled library when `CRUMBS_MAX_HANDLERS` differs.

**When to use:** If you compile CRUMBS as a library separately from your application (e.g., PlatformIO lib_deps), the `CRUMBS_MAX_HANDLERS` value must match between library and application. A mismatch causes memory corruption.

```c
void setup() {
    // Verify ABI compatibility at startup
    if (sizeof(crumbs_context_t) != crumbs_context_size()) {
        Serial.println("ERROR: CRUMBS_MAX_HANDLERS mismatch!");
        Serial.print("Header size: "); Serial.println(sizeof(crumbs_context_t));
        Serial.print("Library size: "); Serial.println(crumbs_context_size());
        while(1); // Halt
    }
}
```

**Fix:** Ensure `CRUMBS_MAX_HANDLERS` is set via `build_flags` in `platformio.ini`, not `#define` before include.

### Command Handler Dispatch

The handler dispatch system provides per-command-type function registration for structured message processing. Instead of (or in addition to) using the general `on_message` callback, you can register individual handlers for specific command types.

```c
/* Handler function signature */
typedef void (*crumbs_handler_fn)(crumbs_context_t *ctx,
                                  uint8_t command_type,
                                  const uint8_t *data,
                                  uint8_t data_len,
                                  void *user_data);

/* Register a handler for command type 0x10 */
crumbs_register_handler(&ctx, 0x10, my_handler, my_userdata);

/* Handler is invoked when that command_type is received */
void my_handler(crumbs_context_t *ctx, uint8_t cmd, const uint8_t *data, uint8_t len, void *ud) {
    // Process command 0x10
}
```

Handler dispatch happens inside `crumbs_peripheral_handle_receive()`:

1. Message is decoded and CRC is validated
2. `on_message` callback is invoked (if set)
3. Registered handler for `msg.command_type` is invoked (if set)

This allows combining both approaches: use `on_message` for logging/statistics while using handlers for command-specific logic.

- `crumbs_register_handler()` returns 0 on success, -1 if ctx is NULL or table full
- Registering with fn=NULL clears the handler (same as `crumbs_unregister_handler()`)
- Overwriting an existing handler replaces it silently

#### Memory Optimization (CRUMBS_MAX_HANDLERS)

By default, the handler table supports 256 commands using O(1) direct lookup. This costs ~1KB on AVR (2-byte pointers) or ~2KB on 32-bit platforms.

For memory-constrained devices, define `CRUMBS_MAX_HANDLERS` before including `crumbs.h`:

```c
#define CRUMBS_MAX_HANDLERS 8  // Only need 8 handlers
#include <crumbs.h>
```

Or via compiler flags (PlatformIO example):

```ini
build_flags = -DCRUMBS_MAX_HANDLERS=8
```

Memory usage by configuration:

| CRUMBS_MAX_HANDLERS | AVR (2-byte ptr) | 32-bit (4-byte ptr) | Lookup |
| ------------------- | ---------------- | ------------------- | ------ |
| 256 (default)       | ~1025 bytes      | ~2049 bytes         | O(1)   |
| 16                  | ~65 bytes        | ~129 bytes          | O(n)   |
| 8                   | ~33 bytes        | ~65 bytes           | O(n)   |
| 0 (disabled)        | 0 bytes          | 0 bytes             | —      |

When < 256, dispatch uses linear search which is fast for typical handler counts (4-16).

---

### Message Builder/Reader Helpers

The `crumbs_msg.h` header provides zero-overhead inline helpers for structured payload construction and reading. These eliminate manual byte manipulation for multi-byte integers and floats.

Include: `#include "crumbs_msg.h"` (Linux) or `#include <crumbs_msg.h>` (Arduino)

```c
/* Initialize a message for building */
static inline void crumbs_msg_init(crumbs_message_t *msg);

/* Add values to message payload (little-endian for integers) */
static inline int crumbs_msg_add_u8(crumbs_message_t *msg, uint8_t value);
static inline int crumbs_msg_add_u16(crumbs_message_t *msg, uint16_t value);
static inline int crumbs_msg_add_u32(crumbs_message_t *msg, uint32_t value);
static inline int crumbs_msg_add_i8(crumbs_message_t *msg, int8_t value);
static inline int crumbs_msg_add_i16(crumbs_message_t *msg, int16_t value);
static inline int crumbs_msg_add_i32(crumbs_message_t *msg, int32_t value);
static inline int crumbs_msg_add_float(crumbs_message_t *msg, float value);  /* native byte order */
static inline int crumbs_msg_add_bytes(crumbs_message_t *msg, const uint8_t *data, size_t len);

/* Read values from payload buffer (used in handlers) */
static inline int crumbs_msg_read_u8(const uint8_t *data, size_t len, size_t *offset, uint8_t *out);
static inline int crumbs_msg_read_u16(const uint8_t *data, size_t len, size_t *offset, uint16_t *out);
static inline int crumbs_msg_read_u32(const uint8_t *data, size_t len, size_t *offset, uint32_t *out);
static inline int crumbs_msg_read_i8(const uint8_t *data, size_t len, size_t *offset, int8_t *out);
static inline int crumbs_msg_read_i16(const uint8_t *data, size_t len, size_t *offset, int16_t *out);
static inline int crumbs_msg_read_i32(const uint8_t *data, size_t len, size_t *offset, int32_t *out);
static inline int crumbs_msg_read_float(const uint8_t *data, size_t len, size_t *offset, float *out);
static inline int crumbs_msg_read_bytes(const uint8_t *data, size_t len, size_t *offset, uint8_t *dest, size_t count);
```

Usage example (controller side):

```c
#include "crumbs_msg.h"

crumbs_message_t msg;
crumbs_msg_init(&msg);
msg.type_id = 0x02;
msg.command_type = 0x01;
crumbs_msg_add_u8(&msg, servo_index);
crumbs_msg_add_u16(&msg, pulse_us);
crumbs_controller_send(&ctx, addr, &msg, write_fn, write_ctx);
```

Usage example (peripheral handler):

```c
void handle_servo(crumbs_context_t *ctx, uint8_t cmd,
                  const uint8_t *data, uint8_t len, void *user) {
    size_t off = 0;
    uint8_t index;
    uint16_t pulse;
    if (crumbs_msg_read_u8(data, len, &off, &index) < 0) return;
    if (crumbs_msg_read_u16(data, len, &off, &pulse) < 0) return;
    set_servo(index, pulse);
}
```

All add/read functions return 0 on success, -1 on bounds overflow. See [Message Helpers](message-helpers.md) for complete documentation.

---

### Return Values and Error Codes

All CRUMBS functions use consistent return value conventions:

- **Success**: `0` (or positive value where documented)
- **Failure**: Negative values indicate specific error conditions

#### Core Functions

| Function                             | Return | Meaning                                                     |
| ------------------------------------ | ------ | ----------------------------------------------------------- |
| `crumbs_encode_message()`            | `>0`   | Encoded frame length (4 + data_len)                         |
|                                      | `0`    | Failure (buffer too small or data_len > CRUMBS_MAX_PAYLOAD) |
| `crumbs_decode_message()`            | `0`    | Success                                                     |
|                                      | `-1`   | Buffer too small, data_len invalid, or truncated frame      |
|                                      | `-2`   | CRC mismatch                                                |
| `crumbs_controller_send()`           | `0`    | Success                                                     |
|                                      | `-1`   | Invalid arguments (NULL ctx/msg/write_fn)                   |
|                                      | `-2`   | Context not in controller role                              |
|                                      | `-3`   | Encode failed                                               |
|                                      | `>0`   | I2C write error (passed through from write_fn)              |
| `crumbs_peripheral_handle_receive()` | `0`    | Success (callbacks invoked)                                 |
|                                      | `-1`   | Invalid arguments or not peripheral role                    |
|                                      | `-1`   | Decode failed (buffer/length error)                         |
|                                      | `-2`   | CRC mismatch                                                |
| `crumbs_peripheral_build_reply()`    | `0`    | Success (out_len set to frame size, or 0 if no reply)       |
|                                      | `-1`   | Invalid arguments or not peripheral role                    |
|                                      | `-2`   | Encode failed                                               |
| `crumbs_register_handler()`          | `0`    | Success                                                     |
|                                      | `-1`   | NULL context or handler table full                          |
| `crumbs_unregister_handler()`        | `0`    | Success (always, even if not found)                         |

#### Linux HAL Functions

| Function                         | Return  | Meaning                                       |
| -------------------------------- | ------- | --------------------------------------------- |
| `crumbs_linux_init_controller()` | `0`     | Success                                       |
|                                  | `-1`    | Invalid arguments                             |
|                                  | `-2`    | Failed to open I2C bus                        |
| `crumbs_linux_i2c_write()`       | `0`     | Success                                       |
|                                  | `-1`    | Invalid arguments or bus not open             |
|                                  | `-2`    | Failed to select slave address                |
|                                  | `-3`    | Write I/O error                               |
|                                  | `-4`    | Incomplete write (bytes written ≠ requested)  |
| `crumbs_linux_read_message()`    | `0`     | Success                                       |
|                                  | `-1`    | Invalid arguments or bus not open             |
|                                  | `-2`    | Failed to select slave address                |
|                                  | `-3`    | Read I/O error                                |
|                                  | `-4`    | No bytes read                                 |
|                                  | `-1/-2` | Decode/CRC error (passed through from decode) |

#### Arduino HAL Functions

| Function                      | Return | Meaning                           |
| ----------------------------- | ------ | --------------------------------- |
| `crumbs_arduino_wire_write()` | `0`    | Success                           |
|                               | `>0`   | Wire.endTransmission() error code |

#### CRC Statistics

When `crumbs_decode_message()` is called with a non-NULL context, it updates CRC statistics:

- `crumbs_get_crc_error_count(ctx)` — total CRC failures since init/reset
- `crumbs_last_crc_ok(ctx)` — 1 if last decode had valid CRC, 0 otherwise
- `crumbs_reset_crc_stats(ctx)` — reset counters to zero

## Arduino HAL (high-level conveniences)

```c
/* Initialize controller/peripheral using default Wire instance */
void crumbs_arduino_init_controller(crumbs_context_t *ctx);
void crumbs_arduino_init_peripheral(crumbs_context_t *ctx, uint8_t address);

/* Wire-based write function for crumbs_controller_send */
int crumbs_arduino_wire_write(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len);
```

Other Arduino helpers:

- `crumbs_arduino_scan()` — TwoWire-compatible bus scanner (strict/non-strict), returns number found.
- `crumbs_arduino_read()` — TwoWire-compatible read helper for `crumbs_i2c_read_fn`.

Usage (controller, Arduino):

```c
crumbs_context_t ctx;
crumbs_arduino_init_controller(&ctx);
// build message with byte payload
crumbs_message_t msg = {0};
msg.type_id = 1;
msg.command_type = 1;
// example: encode a float into bytes
float val = 1.23f;
msg.data_len = sizeof(float);
memcpy(msg.data, &val, sizeof(float));
int rc = crumbs_controller_send(&ctx, 0x08, &msg, crumbs_arduino_wire_write, NULL);
```

Usage (peripheral, Arduino):

```c
crumbs_context_t ctx;
crumbs_arduino_init_peripheral(&ctx, 0x08);
crumbs_set_callbacks(&ctx, on_message_cb, on_request_cb, NULL);
```

## Linux HAL (selected)

> **Note:** The Linux HAL currently supports **controller mode only**. Peripheral mode (acting as an I²C target device) is not yet implemented. For peripheral devices, use an Arduino or other microcontroller.

```c
int crumbs_linux_init_controller(crumbs_context_t *ctx,
                                 crumbs_linux_i2c_t *i2c,
                                 const char *device_path,
                                 uint32_t timeout_us);
void crumbs_linux_close(crumbs_linux_i2c_t *i2c);
int crumbs_linux_i2c_write(void *user_ctx, uint8_t target_addr, const uint8_t *data, size_t len);
int crumbs_linux_read_message(crumbs_linux_i2c_t *i2c, uint8_t target_addr, crumbs_context_t *ctx, crumbs_message_t *out_msg);
```

Return codes and notes (Linux HAL)

- `crumbs_linux_init_controller()` returns 0 on success, -1 for bad args, -2 if opening the bus failed.
- `crumbs_linux_i2c_write()` and `crumbs_linux_read()` will return negative error codes on failure — see `src/crumbs_linux.h` for the small set of linux-wire-mapped errors.

Usage (controller, Linux):

```c
crumbs_context_t ctx;
crumbs_linux_i2c_t bus;
int rc = crumbs_linux_init_controller(&ctx, &bus, "/dev/i2c-1", 10000);
// send using crumbs_controller_send(&ctx, addr, &msg, crumbs_linux_i2c_write, &bus);
```

Note: Most functions return `0` on success and negative or non-zero codes on error — consult the headers for specific return values.

## Scanner & read primitive

The core scanner uses a read primitive (`crumbs_i2c_read_fn`) and an optional write primitive to reliably discover devices that actually speak CRUMBS. It reads up to `CRUMBS_MESSAGE_MAX_SIZE` bytes and only accepts addresses that provide a CRC-valid CRUMBS frame.

Signature (read primitive):

```c
typedef int (*crumbs_i2c_read_fn)(void *user_ctx, uint8_t addr, uint8_t *buffer, size_t len, uint32_t timeout_us);
```

Scanner helper:

```c
int crumbs_controller_scan_for_crumbs(const crumbs_context_t *ctx,
                                     uint8_t start_addr,
                                     uint8_t end_addr,
                                     int strict,
                                     crumbs_i2c_write_fn write_fn,
                                     crumbs_i2c_read_fn read_fn,
                                     void *io_ctx,
                                     uint8_t *found,
                                     size_t max_found,
                                     uint32_t timeout_us);
```

Parameters summary:

- `start_addr`/`end_addr`: inclusive probe range (0x03..0x77 typical)
- `strict`: non-zero => request a data-phase read (strong detection). `0` => lightweight ACK probe.
- `write_fn`/`read_fn` : platform primitives (e.g., `crumbs_arduino_wire_write`, `crumbs_arduino_read`, or `crumbs_linux_i2c_write`, `crumbs_linux_read`).

Return value: number of discovered CRUMBS device addresses (>=0) or negative on error.

Platform-provided read helpers:

- `crumbs_arduino_read()` — TwoWire-based read implementation for Arduino.
- `crumbs_linux_read()` — linux-wire based read implementation for native Linux.

---

If you need a quick reference to these types and function signatures for direct inclusion into code, the public headers in `src/` are intentionally stable and self-documenting; use them as authoritative API. The summary above is complete enough to consume the library from a binary/compressed distribution (static archive + headers) without reading implementation files.
