# API Reference

This file documents the public CRUMBS C API (core + provided HAL adapters) so you can use CRUMBS as a compiled library without reading the implementation.

The core is intentionally small and C-friendly so it is easy to consume from Arduino sketches, PlatformIO projects or native Linux programs.

## Key types and constants

- `crumbs_message_t` — fixed-size message struct (see `src/crumbs_message.h`). Fields:

  - `slice_address` — (logical) slice address (NOT serialized)
  - `type_id` — 1 byte
  - `command_type` — 1 byte
  - `data[7]` — 7 float32 payload fields (little-endian)
  - `crc8` — 1 byte CRC-8 computed over the serialized payload

- `crumbs_context_t` — per-endpoint context (role, address, CRC statistics, callbacks, user_data)
- `CRUMBS_DATA_LENGTH` — number of payload floats (7)
- `CRUMBS_MESSAGE_SIZE` — serialized frame size (31 bytes)

Serialized frame layout (31 bytes): - type_id (1) - command_type (1) - data (7 \* 4 = 28) - crc8 (1)

## Core API (full, concise)

```c
/* Initialize a context */
void crumbs_init(crumbs_context_t *ctx, crumbs_role_t role, uint8_t address);

/* Install callbacks */
void crumbs_set_callbacks(crumbs_context_t *ctx,
                          crumbs_message_cb_t on_message,
                          crumbs_request_cb_t on_request,
                          void *user_data);

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

---

### Return values and common semantics

- Encoding/decoding
    - `crumbs_encode_message()` returns CRUMBS_MESSAGE_SIZE on success, 0 on failure (e.g., buffer too small).
    - `crumbs_decode_message()` returns `0` on success, `-1` if buffer too small, and `-2` if the CRC check fails. When a non-NULL context is passed the function will update CRC statistics accessible via `crumbs_get_crc_error_count()` and `crumbs_last_crc_ok()`.

- Controller helpers
    - `crumbs_controller_send()` returns `0` on success; non-zero indicates an I2C write error as returned by the platform `write_fn`.

- Peripheral helpers
    - `crumbs_peripheral_handle_receive()` returns `0` on success, negative on decode/CRC errors. When successful it will invoke `ctx->on_message` if configured.
    - `crumbs_peripheral_build_reply()` returns `0` on success. If no reply is available the function returns 0 and sets `out_len` to 0.

All HAL adapters follow the `crumbs_i2c_*` function signatures in `src/crumbs_i2c.h` — write functions should return 0 on success and non-zero on error; read functions should return number-of-bytes-read (>=0) or negative on error.
```

## Arduino HAL (high-level conveniences)

````c
/* Initialize controller/peripheral using default Wire instance */
void crumbs_arduino_init_controller(crumbs_context_t *ctx);
void crumbs_arduino_init_peripheral(crumbs_context_t *ctx, uint8_t address);

/* Wire-based write function for crumbs_controller_send */
int crumbs_arduino_wire_write(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len);

Other Arduino helpers:
- `crumbs_arduino_scan()` — TwoWire-compatible bus scanner (strict/non-strict), returns number found.
- `crumbs_arduino_read()` — TwoWire-compatible read helper for `crumbs_i2c_read_fn`.

Usage (controller, Arduino):

```c
crumbs_context_t ctx;
crumbs_arduino_init_controller(&ctx);
// build message
crumbs_message_t msg = {0}; msg.type_id=1; msg.command_type=1; msg.data[0]=1.23f;
int rc = crumbs_controller_send(&ctx, 0x08, &msg, crumbs_arduino_wire_write, NULL);
````

Usage (peripheral, Arduino):

```c
crumbs_context_t ctx;
crumbs_arduino_init_peripheral(&ctx, 0x08);
crumbs_set_callbacks(&ctx, on_message_cb, on_request_cb, NULL);
```

````

## Linux HAL (selected)

```c
int crumbs_linux_init_controller(crumbs_context_t *ctx,
                                 crumbs_linux_i2c_t *i2c,
                                 const char *device_path,
                                 uint32_t timeout_us);
void crumbs_linux_close(crumbs_linux_i2c_t *i2c);
int crumbs_linux_i2c_write(void *user_ctx, uint8_t target_addr, const uint8_t *data, size_t len);
int crumbs_linux_read_message(crumbs_linux_i2c_t *i2c, uint8_t target_addr, crumbs_context_t *ctx, crumbs_message_t *out_msg);

Return codes and notes (Linux HAL)
- `crumbs_linux_init_controller()` returns 0 on success, -1 for bad args, -2 if opening the bus failed.
- `crumbs_linux_i2c_write()` and `crumbs_linux_read()` will return negative error codes on failure — see `src/crumbs_linux.h` for the small set of linux-wire-mapped errors.

Usage (controller, Linux):

```c
crumbs_context_t ctx;
crumbs_linux_i2c_t bus;
int rc = crumbs_linux_init_controller(&ctx, &bus, "/dev/i2c-1", 10000);
// send using crumbs_controller_send(&ctx, addr, &msg, crumbs_linux_i2c_write, &bus);
````

````

Note: Most functions return `0` on success and negative or non-zero codes on error — consult the headers for specific return values.

## Scanner & read primitive

The core scanner uses a read primitive (`crumbs_i2c_read_fn`) and an optional write primitive to reliably discover devices that actually speak CRUMBS. It reads up to `CRUMBS_MESSAGE_SIZE` bytes and only accepts addresses that provide a CRC-valid CRUMBS frame.

Signature (read primitive):

```c
typedef int (*crumbs_i2c_read_fn)(void *user_ctx, uint8_t addr, uint8_t *buffer, size_t len, uint32_t timeout_us);
````

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
