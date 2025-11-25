# API Reference (C core + platform HALs)

This section documents the public C API exposed by the CRUMBS core and the platform HALs (Arduino and Linux).

The core is intentionally C-friendly and small so it is easy to reuse in both Arduino and native Linux code.

## Key types and constants

- `crumbs_message_t` — fixed-size message struct (see `src/crumbs_message.h`). Fields include `slice_address`, `type_id`, `command_type`, `data[7]`, `crc8`.
- `crumbs_context_t` — per-endpoint context used by the core (role, address, CRC stats, callbacks).
- `CRUMBS_DATA_LENGTH` — number of float payload elements (7).
- `CRUMBS_MESSAGE_SIZE` — serialized frame size in bytes (31).

**Serialized frame**: 31 bytes (type_id + command_type + data + crc8)

## Core API (selected)

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
```

## Arduino HAL (high level)

```c
/* Initialize controller/peripheral using default Wire instance */
void crumbs_arduino_init_controller(crumbs_context_t *ctx);
void crumbs_arduino_init_peripheral(crumbs_context_t *ctx, uint8_t address);

/* Wire-based write function for crumbs_controller_send */
int crumbs_arduino_wire_write(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len);
```

## Linux HAL (selected)

```c
int crumbs_linux_init_controller(crumbs_context_t *ctx,
                                 crumbs_linux_i2c_t *i2c,
                                 const char *device_path,
                                 uint32_t timeout_us);
void crumbs_linux_close(crumbs_linux_i2c_t *i2c);
int crumbs_linux_i2c_write(void *user_ctx, uint8_t target_addr, const uint8_t *data, size_t len);
int crumbs_linux_read_message(crumbs_linux_i2c_t *i2c, uint8_t target_addr, crumbs_context_t *ctx, crumbs_message_t *out_msg);
```

Note: Most functions return `0` on success and negative or non-zero codes on error — consult the headers for specific return values.
