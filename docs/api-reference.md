# API Reference

This document provides the complete CRUMBS C API reference for users implementing controllers and peripherals. All public functions, types, constants, and helper utilities are documented here.

## Overview

The CRUMBS API is organized into:

- **Core API**: Message encoding/decoding, context management, callbacks
- **Handler Dispatch**: Per-command function registration for peripherals
- **Message Helpers**: Type-safe payload builders and readers
- **Platform HAL**: Arduino and Linux adapter functions
- **Discovery**: Bus scanning for CRUMBS-compatible devices

## Key Types and Constants

### Message Structure

```c
typedef struct {
    uint8_t address;      // Device address (not serialized, for context only)
    uint8_t type_id;      // Device/module type identifier
    uint8_t opcode;       // Command/query opcode
    uint8_t data_len;     // Payload length (0-27)
    uint8_t data[27];     // Payload buffer
    uint8_t crc8;         // CRC-8 checksum over serialized frame
} crumbs_message_t;
```

**Serialized frame format** (4-31 bytes):

```
[type_id:1][opcode:1][data_len:1][data:0-27][crc8:1]
```

CRC is computed over: `type_id + opcode + data_len + data[0..data_len-1]`

### Context Structure

```c
typedef struct crumbs_context_t {
    crumbs_role_t role;           // CONTROLLER or PERIPHERAL
    uint8_t address;              // Device I²C address (peripherals only)

    // CRC statistics
    uint32_t crc_error_count;     // Cumulative CRC failures
    int last_crc_ok;              // 1 if last decode succeeded, 0 otherwise

    // Callbacks
    crumbs_message_cb_t on_message;    // General message callback
    crumbs_request_cb_t on_request;    // Peripheral request callback (for reads)
    void *user_data;                   // Opaque user pointer

    // Handler dispatch table (if enabled)
    // ... internal fields ...
} crumbs_context_t;
```

### Constants

```c
#define CRUMBS_MAX_PAYLOAD      27    // Maximum payload bytes
#define CRUMBS_MESSAGE_MAX_SIZE 31    // Maximum serialized frame size
#define CRUMBS_VERSION          0x0A03 // Library version (0x0A03 = v0.10.3)
```

### Callback Signatures

```c
// Message callback (invoked on all received messages)
typedef void (*crumbs_message_cb_t)(
    crumbs_context_t *ctx,
    const crumbs_message_t *msg);

// Request callback (invoked when peripheral receives I²C read request)
typedef void (*crumbs_request_cb_t)(
    crumbs_context_t *ctx,
    crumbs_message_t *reply);

// Command handler (per-opcode dispatch)
typedef void (*crumbs_handler_fn)(
    crumbs_context_t *ctx,
    uint8_t opcode,
    const uint8_t *data,
    uint8_t data_len,
    void *user_data);
```

---

## Core API

### Context Initialization

```c
void crumbs_init(crumbs_context_t *ctx, crumbs_role_t role, uint8_t address);
```

Initialize a CRUMBS context. Must be called before any other operations.

**Parameters:**

- `ctx` — Context to initialize
- `role` — `CRUMBS_ROLE_CONTROLLER` or `CRUMBS_ROLE_PERIPHERAL`
- `address` — Device I²C address (only used for peripherals; controllers can use 0x00)

### Callback Registration

```c
void crumbs_set_callbacks(crumbs_context_t *ctx,
                          crumbs_message_cb_t on_message,
                          crumbs_request_cb_t on_request,
                          void *user_data);
```

Install callbacks for message handling.

**Parameters:**

- `on_message` — Invoked when a message is received (both roles). May be NULL.
- `on_request` — Invoked when peripheral receives an I²C read request. May be NULL.
- `user_data` — Opaque pointer passed to callbacks.

**Callback Execution Order:**

1. Message decoded and CRC validated
2. `on_message` callback invoked (if registered)
3. Handler dispatch (if registered for this opcode)

### Encoding and Decoding

```c
size_t crumbs_encode_message(const crumbs_message_t *msg,
                             uint8_t *buffer,
                             size_t buffer_len);
```

Encode a message into a wire-format frame.

**Returns:**

- Encoded frame length (4 + data_len) on success
- `0` on failure (buffer too small or data_len > 27)

---

```c
int crumbs_decode_message(const uint8_t *buffer,
                          size_t buffer_len,
                          crumbs_message_t *msg,
                          crumbs_context_t *ctx);
```

Decode a wire-format frame into a message structure.

**Parameters:**

- `ctx` — Optional context for CRC statistics (may be NULL)

**Returns:**

- `0` — Success
- `-1` — Invalid frame (too short, bad data_len, truncated)
- `-2` — CRC mismatch

If `ctx` is non-NULL, CRC statistics are updated.

### Controller Operations

```c
int crumbs_controller_send(const crumbs_context_t *ctx,
                           uint8_t target_addr,
                           const crumbs_message_t *msg,
                           crumbs_i2c_write_fn write_fn,
                           void *write_ctx);
```

Send a message from controller to a peripheral device.

**Returns:**

- `0` — Success
- `-1` — Invalid arguments (NULL ctx/msg/write_fn)
- `-2` — Context not in controller role
- `-3` — Encode failed
- `>0` — I2C write error (from write_fn)

**Example:**

```c
crumbs_message_t msg;
crumbs_msg_init(&msg, 0x01, 0x10);  // type=LED, opcode=query
int rc = crumbs_controller_send(&ctx, 0x08, &msg,
                                crumbs_arduino_wire_write, NULL);
```

### Peripheral Operations

```c
int crumbs_peripheral_handle_receive(crumbs_context_t *ctx,
                                     const uint8_t *buffer,
                                     size_t len);
```

Process incoming data on a peripheral device. Decodes the message, validates CRC, and invokes callbacks/handlers.

**Returns:**

- `0` — Success (callbacks invoked)
- `-1` — Invalid arguments, not peripheral role, or decode failed
- `-2` — CRC mismatch

Typically called from Wire `onReceive()` callback on Arduino.

---

```c
int crumbs_peripheral_build_reply(crumbs_context_t *ctx,
                                  uint8_t *out_buf,
                                  size_t out_buf_len,
                                  size_t *out_len);
```

Build a reply frame for an I²C read request. Invokes the `on_request` callback to populate the reply.

**Returns:**

- `0` — Success (out_len set to frame size, or 0 if no reply)
- `-1` — Invalid arguments or not peripheral role
- `-2` — Encode failed

Typically called from Wire `onRequest()` callback on Arduino.

### CRC Statistics

```c
uint32_t crumbs_get_crc_error_count(const crumbs_context_t *ctx);
int crumbs_last_crc_ok(const crumbs_context_t *ctx);
void crumbs_reset_crc_stats(crumbs_context_t *ctx);
```

Access CRC validation statistics. Use these for diagnostics or to detect noisy I²C bus conditions.

### ABI Compatibility Check

```c
size_t crumbs_context_size(void);
```

Returns the size of `crumbs_context_t` as compiled into the library. Use this to detect mismatches when `CRUMBS_MAX_HANDLERS` differs between library and application.

**When to use:** Precompiled libraries (PlatformIO lib_deps)

**Example:**

```c
void setup() {
    if (sizeof(crumbs_context_t) != crumbs_context_size()) {
        Serial.println("ERROR: CRUMBS_MAX_HANDLERS mismatch!");
        while(1);  // Halt
    }
}
```

**Fix:** Set `CRUMBS_MAX_HANDLERS` via `build_flags` in `platformio.ini`.

---

## Handler Dispatch

The handler dispatch system provides per-opcode function registration for structured command processing.

### Handler Registration

```c
int crumbs_register_handler(crumbs_context_t *ctx,
                            uint8_t opcode,
                            crumbs_handler_fn fn,
                            void *user_data);
```

Register a handler function for a specific opcode.

**Returns:**

- `0` — Success
- `-1` — NULL context or handler table full

**Example:**

```c
crumbs_register_handler(&ctx, 0x01, handle_led_set, &led_state);
crumbs_register_handler(&ctx, 0x10, handle_led_query, NULL);
```

---

```c
int crumbs_unregister_handler(crumbs_context_t *ctx, uint8_t opcode);
```

Remove a handler for a specific opcode. Always returns `0`.

Alternatively, call `crumbs_register_handler()` with `fn = NULL`.

### Handler Function Signature

```c
void my_handler(crumbs_context_t *ctx,
                uint8_t opcode,
                const uint8_t *data,
                uint8_t data_len,
                void *user_data) {
    // Process command...
}
```

**Parameters:**

- `ctx` — The CRUMBS context
- `opcode` — The command that triggered this handler
- `data` — Payload bytes (may be NULL if data_len == 0)
- `data_len` — Payload length (0-27)
- `user_data` — Opaque pointer registered with this handler

### Handler Dispatch Flow

When a message arrives at a peripheral:

1. Message decoded and CRC validated
2. `on_message` callback invoked (if registered)
3. Handler dispatch searches for matching opcode
4. Handler invoked (if found)

Both mechanisms can coexist — use `on_message` for logging, handlers for command logic.

### Memory Configuration

Handler dispatch uses `CRUMBS_MAX_HANDLERS` slots (default: 16).

**Adjust handler table size:**

```ini
# platformio.ini
build_flags = -DCRUMBS_MAX_HANDLERS=8
```

**Disable handler dispatch:**

```ini
build_flags = -DCRUMBS_MAX_HANDLERS=0
```

> **Important:** Always set via `build_flags`. Defining in sketch before `#include` does not work on Arduino/PlatformIO due to separate library compilation.

**Memory usage:**

| Max Handlers | AVR (2-byte ptr) | 32-bit (4-byte ptr) |
| ------------ | ---------------- | ------------------- |
| 16           | ~68 bytes        | ~132 bytes          |
| 8            | ~36 bytes        | ~68 bytes           |
| 4            | ~21 bytes        | ~37 bytes           |
| 0            | 0 bytes          | 0 bytes             |

---

## Message Helpers

The `crumbs_message_helpers.h` header provides zero-overhead inline helpers for building and reading message payloads. These eliminate manual byte manipulation.

**Include:**

```c
#include "crumbs_message_helpers.h"    // Linux/CMake
#include <crumbs_message_helpers.h>    // Arduino
```

### Message Builder

#### Initialization

```c
void crumbs_msg_init(crumbs_message_t *msg, uint8_t type_id, uint8_t opcode);
```

Initialize a message with type and opcode. Sets `data_len = 0` and zeros all fields.

#### Adding Values

All add functions return `0` on success, `-1` if the value would exceed the 27-byte payload limit.

```c
int crumbs_msg_add_u8(crumbs_message_t *msg, uint8_t value);
int crumbs_msg_add_u16(crumbs_message_t *msg, uint16_t value);     // little-endian
int crumbs_msg_add_u32(crumbs_message_t *msg, uint32_t value);     // little-endian
int crumbs_msg_add_i8(crumbs_message_t *msg, int8_t value);
int crumbs_msg_add_i16(crumbs_message_t *msg, int16_t value);      // little-endian
int crumbs_msg_add_i32(crumbs_message_t *msg, int32_t value);      // little-endian
int crumbs_msg_add_float(crumbs_message_t *msg, float value);      // native byte order
int crumbs_msg_add_bytes(crumbs_message_t *msg, const void *data, uint8_t len);
```

**Encoding notes:**

- Multi-byte integers use **little-endian** (LSB first)
- Floats use **native byte order** (memcpy)

**Example:**

```c
crumbs_message_t msg;
crumbs_msg_init(&msg, 0x02, 0x02);  // type=Servo, opcode=SetBoth
crumbs_msg_add_u16(&msg, 1500);     // Servo 1
crumbs_msg_add_u16(&msg, 2000);     // Servo 2
```

### Message Reader

All read functions return `0` on success, `-1` if reading would exceed buffer bounds.

```c
int crumbs_msg_read_u8(const uint8_t *data, uint8_t len, uint8_t offset, uint8_t *out);
int crumbs_msg_read_u16(const uint8_t *data, uint8_t len, uint8_t offset, uint16_t *out);
int crumbs_msg_read_u32(const uint8_t *data, uint8_t len, uint8_t offset, uint32_t *out);
int crumbs_msg_read_i8(const uint8_t *data, uint8_t len, uint8_t offset, int8_t *out);
int crumbs_msg_read_i16(const uint8_t *data, uint8_t len, uint8_t offset, int16_t *out);
int crumbs_msg_read_i32(const uint8_t *data, uint8_t len, uint8_t offset, int32_t *out);
int crumbs_msg_read_float(const uint8_t *data, uint8_t len, uint8_t offset, float *out);
int crumbs_msg_read_bytes(const uint8_t *data, uint8_t len, uint8_t offset, void *out, uint8_t count);
```

**Example (peripheral handler):**

```c
void handle_servo(crumbs_context_t *ctx, uint8_t cmd,
                  const uint8_t *data, uint8_t len, void *user) {
    uint8_t channel;
    uint16_t pulse_us;

    if (crumbs_msg_read_u8(data, len, 0, &channel) < 0) return;
    if (crumbs_msg_read_u16(data, len, 1, &pulse_us) < 0) return;

    set_servo(channel, pulse_us);
}
```

### Command Header Pattern

For reusable command definitions, create a shared header file. See `examples/handlers_usage/mock_ops.h` for a complete example.

**Pattern:**

```c
// my_device_commands.h
#ifndef MY_DEVICE_COMMANDS_H
#define MY_DEVICE_COMMANDS_H

#include "crumbs.h"
#include "crumbs_message_helpers.h"

#define MY_TYPE_ID        0x10
#define MY_CMD_ACTION_A   0x01
#define MY_CMD_ACTION_B   0x02

static inline int my_send_action_a(
    crumbs_context_t *ctx, uint8_t addr,
    uint16_t param1, uint8_t param2,
    crumbs_i2c_write_fn write_fn, void *write_ctx) {

    crumbs_message_t msg;
    crumbs_msg_init(&msg, MY_TYPE_ID, MY_CMD_ACTION_A);
    crumbs_msg_add_u16(&msg, param1);
    crumbs_msg_add_u8(&msg, param2);

    return crumbs_controller_send(ctx, addr, &msg, write_fn, write_ctx);
}

#endif
```

**Benefits:**

- Type-safe: Function parameters enforce correct types
- Self-documenting: Command names and parameters are explicit
- Reusable: Same header works on controller and peripheral
- Composable: Include multiple device headers in one controller

---

## Platform HAL: Arduino

### Initialization

```c
void crumbs_arduino_init_controller(crumbs_context_t *ctx);
void crumbs_arduino_init_peripheral(crumbs_context_t *ctx, uint8_t address);
```

Initialize context and register Wire callbacks. Uses default Wire instance.

**Example (controller):**

```c
crumbs_context_t ctx;
crumbs_arduino_init_controller(&ctx);
```

**Example (peripheral):**

```c
#define I2C_ADDRESS 0x08
crumbs_context_t ctx;
crumbs_arduino_init_peripheral(&ctx, I2C_ADDRESS);
crumbs_set_callbacks(&ctx, NULL, on_request, NULL);
```

### I²C Write Function

```c
int crumbs_arduino_wire_write(void *user_ctx, uint8_t addr,
                              const uint8_t *data, size_t len);
```

Wire-based write function for `crumbs_controller_send()`.

**Returns:**

- `0` — Success
- `>0` — Wire error code (from `endTransmission()`)

**Example:**

```c
crumbs_message_t msg;
crumbs_msg_init(&msg, 0x01, 0x01);
crumbs_controller_send(&ctx, 0x08, &msg, crumbs_arduino_wire_write, NULL);
```

### I²C Read Function

```c
int crumbs_arduino_read(void *user_ctx, uint8_t addr,
                       uint8_t *buffer, size_t len, uint32_t timeout_us);
```

Wire-based read function for scanner and diagnostics.

**Returns:**

- Number of bytes read (0-31)
- Negative values on error

### Bus Scanner

```c
int crumbs_arduino_scan(TwoWire *wire, uint8_t start_addr, uint8_t end_addr,
                       int strict, uint8_t *found, size_t max_found);
```

Scan I²C bus for CRUMBS-compatible devices.

**Parameters:**

- `strict` — Non-zero: requires valid CRUMBS frame. Zero: ACK-based detection.
- `found` — Output array for discovered addresses
- `max_found` — Size of found array

**Returns:** Number of devices found

**Example:**

```c
uint8_t devices[10];
int count = crumbs_arduino_scan(&Wire, 0x08, 0x77, 1, devices, 10);
```

---

## Platform HAL: Linux

> **Note:** Linux HAL supports **controller mode only**. Peripheral mode (I²C target) is not yet implemented.

### Initialization

```c
int crumbs_linux_init_controller(crumbs_context_t *ctx,
                                 crumbs_linux_i2c_t *i2c,
                                 const char *device_path,
                                 uint32_t timeout_us);
```

Initialize controller context and open I²C bus device.

**Parameters:**

- `device_path` — e.g., `"/dev/i2c-1"`
- `timeout_us` — Read timeout in microseconds

**Returns:**

- `0` — Success
- `-1` — Invalid arguments
- `-2` — Failed to open device

**Example:**

```c
crumbs_context_t ctx;
crumbs_linux_i2c_t bus;
int rc = crumbs_linux_init_controller(&ctx, &bus, "/dev/i2c-1", 10000);
```

### Cleanup

```c
void crumbs_linux_close(crumbs_linux_i2c_t *i2c);
```

Close I²C bus file descriptor.

### I²C Write Function

```c
int crumbs_linux_i2c_write(void *user_ctx, uint8_t target_addr,
                           const uint8_t *data, size_t len);
```

Linux I²C write function for `crumbs_controller_send()`.

**Returns:**

- `0` — Success
- `-1` — Invalid arguments or bus not open
- `-2` — Failed to select slave address
- `-3` — Write I/O error
- `-4` — Incomplete write

### I²C Read Function

```c
int crumbs_linux_read_message(crumbs_linux_i2c_t *i2c, uint8_t target_addr,
                              crumbs_context_t *ctx, crumbs_message_t *out_msg);
```

Read and decode a CRUMBS message from a peripheral.

**Returns:**

- `0` — Success
- `-1` — Invalid arguments or bus not open
- `-2` — Failed to select slave address
- `-3` — Read I/O error
- `-4` — No bytes read
- `-1/-2` — Decode/CRC error (from decode)

---

## Discovery and Scanning

### Core Scanner

```c
int crumbs_controller_scan_for_crumbs(
    const crumbs_context_t *ctx,
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

Platform-independent bus scanner for CRUMBS-compatible devices.

**Parameters:**

- `start_addr`/`end_addr` — Inclusive probe range (0x08-0x77 typical)
- `strict` — Non-zero: request data-phase read (strong detection). Zero: ACK probe only.
- `write_fn`/`read_fn` — Platform primitives
- `found` — Output array for discovered addresses
- `max_found` — Size of found array
- `timeout_us` — Read timeout per device

**Returns:** Number of devices found, or negative on error

**Example (Arduino):**

```c
uint8_t devices[10];
int count = crumbs_controller_scan_for_crumbs(
    &ctx, 0x08, 0x77, 1,
    crumbs_arduino_wire_write, crumbs_arduino_read, NULL,
    devices, 10, 10000);
```

---

## Return Values and Error Codes

All CRUMBS functions use consistent conventions:

- **Success**: `0` (or positive value where documented)
- **Failure**: Negative values indicate specific error conditions

### Core Functions

| Function                             | Success             | Error                                                     |
| ------------------------------------ | ------------------- | --------------------------------------------------------- |
| `crumbs_encode_message()`            | `>0` (frame length) | `0` (buffer too small)                                    |
| `crumbs_decode_message()`            | `0`                 | `-1` (frame error), `-2` (CRC mismatch)                   |
| `crumbs_controller_send()`           | `0`                 | `-1` (args), `-2` (role), `-3` (encode), `>0` (I2C error) |
| `crumbs_peripheral_handle_receive()` | `0`                 | `-1` (args/decode), `-2` (CRC)                            |
| `crumbs_peripheral_build_reply()`    | `0`                 | `-1` (args/role), `-2` (encode)                           |
| `crumbs_register_handler()`          | `0`                 | `-1` (NULL ctx or table full)                             |
| `crumbs_unregister_handler()`        | `0`                 | Never fails                                               |

### Arduino HAL

| Function                      | Success | Error                  |
| ----------------------------- | ------- | ---------------------- |
| `crumbs_arduino_wire_write()` | `0`     | `>0` (Wire error code) |

### Linux HAL

| Function                         | Success | Error                                                                 |
| -------------------------------- | ------- | --------------------------------------------------------------------- |
| `crumbs_linux_init_controller()` | `0`     | `-1` (args), `-2` (open failed)                                       |
| `crumbs_linux_i2c_write()`       | `0`     | `-1` (args), `-2` (select), `-3` (I/O), `-4` (incomplete)             |
| `crumbs_linux_read_message()`    | `0`     | `-1` (args), `-2` (select), `-3` (I/O), `-4` (no data), decode errors |

---

## Complete Examples

### Peripheral with Handler Dispatch

```cpp
#include <crumbs.h>
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

#define I2C_ADDRESS 0x08
static crumbs_context_t ctx;
static uint8_t led_state = 0;

void handle_set_all(crumbs_context_t *c, uint8_t cmd,
                    const uint8_t *data, uint8_t len, void *user) {
    uint8_t bitmask;
    if (crumbs_msg_read_u8(data, len, 0, &bitmask) == 0) {
        led_state = bitmask;
    }
}

void handle_set_one(crumbs_context_t *c, uint8_t cmd,
                    const uint8_t *data, uint8_t len, void *user) {
    uint8_t index, state;
    if (crumbs_msg_read_u8(data, len, 0, &index) == 0 &&
        crumbs_msg_read_u8(data, len, 1, &state) == 0) {
        if (state) led_state |= (1 << index);
        else led_state &= ~(1 << index);
    }
}

void on_request(crumbs_context_t *ctx, crumbs_message_t *reply) {
    if (ctx->requested_opcode == 0x10) {  // Query state
        crumbs_msg_init(reply, 0x01, 0x10);
        crumbs_msg_add_u8(reply, led_state);
    }
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, I2C_ADDRESS);
    crumbs_register_handler(&ctx, 0x01, handle_set_all, NULL);
    crumbs_register_handler(&ctx, 0x02, handle_set_one, NULL);
    crumbs_set_callbacks(&ctx, NULL, on_request, NULL);
}

void loop() {}
```

### Controller with Message Helpers

```cpp
#include <crumbs.h>
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

static crumbs_context_t ctx;

void setup() {
    Serial.begin(9600);
    crumbs_arduino_init_controller(&ctx);
}

void loop() {
    // Send LED command
    crumbs_message_t msg;
    crumbs_msg_init(&msg, 0x01, 0x02);  // type=LED, opcode=set_one
    crumbs_msg_add_u8(&msg, 0);         // LED index 0
    crumbs_msg_add_u8(&msg, 1);         // State ON

    int rc = crumbs_controller_send(&ctx, 0x08, &msg,
                                    crumbs_arduino_wire_write, NULL);

    if (rc != 0) {
        Serial.print("Send failed: ");
        Serial.println(rc);
    }

    delay(1000);
}
```

---

## See Also

- [Protocol Specification](protocol.md) — Wire format, versioning, CRC-8
- [Platform Setup](platform-setup.md) — Installation and configuration
- [Examples](examples.md) — Complete working examples
- Source headers: `src/crumbs.h`, `src/crumbs_message_helpers.h`, `src/crumbs_arduino.h`, `src/crumbs_linux.h`
