# Creating a CRUMBS Device Family

A **family** is a named collection of CRUMBS peripheral types that share a common firmware base. For each type in the family you write one `*_ops.h` header that encapsulates every command the controller can send or query. This keeps application code clean and platform-agnostic.

This guide walks through creating a complete ops header from scratch, using a toy two-channel thermometer (`therm_ops.h`) as the running example.

---

## Anatomy of an ops header

An ops header is a **single C header** containing:

1. A `#define` for your device's `TYPE_ID`
2. `#define` constants for every opcode
3. `static inline` sender functions (`*_send_*`) — one per SET command
4. `static inline` query functions (`*_query_*`) — one per GET command (sends the SET_REPLY trigger)
5. Result structs (`*_result_t`) — one per GET command
6. `static inline` combined get functions (`*_get_*`) — query + delay + read + parse in one call

All functions are `static inline` so the header is self-contained — no `.c` file is needed.

---

## Step 1: Choose a type ID and opcodes

Pick a type ID that does not conflict with existing families. The LHWIT family uses 0x01–0x04. Document your choices clearly.

```c
/** @brief Type ID for 2-channel thermometer device. */
#define THERM_TYPE_ID 0x07

/* SET operations */
#define THERM_OP_SET_SAMPLE_RATE 0x01   /**< Set samples/second [rate:u8] */

/* GET operations (via SET_REPLY) */
#define THERM_OP_GET_TEMP        0x80   /**< Get both temperatures [ch0:i16][ch1:i16] */
#define THERM_OP_GET_SAMPLE_RATE 0x81   /**< Get current sample rate [rate:u8] */
```

Convention: SET opcodes start at 0x01, GET opcodes start at 0x80. This avoids the reserved opcode 0xFE (SET_REPLY).

---

## Step 2: Write the boilerplate

```c
#ifndef THERM_OPS_H
#define THERM_OPS_H

#include "crumbs.h"
#include "crumbs_message_helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ... your type ID and opcode defines here ... */

/* ... your functions and structs here ... */

#ifdef __cplusplus
}
#endif

#endif /* THERM_OPS_H */
```

---

## Step 3: Write the sender functions

One sender per SET command. Use `crumbs_msg_init` + `crumbs_msg_add_*` helpers to build the payload, then `crumbs_controller_send` to transmit.

```c
/**
 * @brief Set thermometer sample rate.
 * @param ctx      Controller context.
 * @param addr     I2C address of thermometer peripheral.
 * @param write_fn I2C write function (e.g. crumbs_linux_i2c_write).
 * @param io       I2C context (e.g. &linux_i2c handle, or NULL for Arduino Wire).
 * @param rate     Samples per second (1–10).
 * @return 0 on success.
 */
static inline int therm_send_set_sample_rate(crumbs_context_t *ctx,
                                             uint8_t addr,
                                             crumbs_i2c_write_fn write_fn,
                                             void *io,
                                             uint8_t rate)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, THERM_TYPE_ID, THERM_OP_SET_SAMPLE_RATE);
    crumbs_msg_add_u8(&msg, rate);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}
```

---

## Step 4: Write the query functions

One query function per GET command. It sends the SET_REPLY trigger that tells the peripheral which opcode to include in its next read response.

```c
/**
 * @brief Request current temperatures (both channels).
 * Peripheral will stage a reply on the next I2C read.
 */
static inline int therm_query_temp(crumbs_context_t *ctx,
                                   uint8_t addr,
                                   crumbs_i2c_write_fn write_fn,
                                   void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, 0, CRUMBS_CMD_SET_REPLY);   /* type_id=0 is fine for SET_REPLY */
    crumbs_msg_add_u8(&msg, THERM_OP_GET_TEMP);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

static inline int therm_query_sample_rate(crumbs_context_t *ctx,
                                          uint8_t addr,
                                          crumbs_i2c_write_fn write_fn,
                                          void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, 0, CRUMBS_CMD_SET_REPLY);
    crumbs_msg_add_u8(&msg, THERM_OP_GET_SAMPLE_RATE);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}
```

---

## Step 5: Define result structs

One struct per GET command, matching the wire payload exactly.

```c
/**
 * @brief Result of THERM_OP_GET_TEMP.
 * Wire payload: [ch0:i16][ch1:i16] (little-endian, 0.01 °C units)
 */
typedef struct {
    int16_t temp_ch0; /**< Channel 0 temperature in 0.01 °C units. */
    int16_t temp_ch1; /**< Channel 1 temperature in 0.01 °C units. */
} therm_temp_result_t;

/**
 * @brief Result of THERM_OP_GET_SAMPLE_RATE.
 */
typedef struct {
    uint8_t rate; /**< Samples per second. */
} therm_sample_rate_result_t;
```

---

## Step 6: Write the combined `_get_*` functions

This is the key abstraction. Each `_get_*` function performs the full three-step flow internally:

1. `*_query_*()` — sends the SET_REPLY write
2. `delay_fn(CRUMBS_DEFAULT_QUERY_DELAY_US)` — waits for the peripheral to stage its reply
3. `crumbs_controller_read()` — reads + decodes the reply frame
4. Parse the payload into the result struct

```c
/**
 * @brief Read both temperature channels from the thermometer.
 *
 * @param ctx      Controller context.
 * @param addr     I2C address of thermometer peripheral.
 * @param write_fn I2C write function.
 * @param read_fn  I2C read function (e.g. crumbs_linux_read).
 * @param delay_fn Platform microsecond delay (e.g. crumbs_linux_delay_us).
 * @param io       I2C context.
 * @param out      Output struct (must not be NULL).
 * @return 0 on success, non-zero on error.
 */
static inline int therm_get_temp(crumbs_context_t *ctx,
                                 uint8_t addr,
                                 crumbs_i2c_write_fn write_fn,
                                 crumbs_i2c_read_fn read_fn,
                                 crumbs_delay_fn delay_fn,
                                 void *io,
                                 therm_temp_result_t *out)
{
    crumbs_message_t reply;
    int rc;
    if (!out)
        return -1;

    rc = therm_query_temp(ctx, addr, write_fn, io);
    if (rc != 0)
        return rc;

    delay_fn(CRUMBS_DEFAULT_QUERY_DELAY_US);

    rc = crumbs_controller_read(ctx, addr, &reply, read_fn, io);
    if (rc != 0)
        return rc;

    /* Parse [ch0:i16][ch1:i16] using crumbs_msg_read_u16 then cast */
    uint16_t raw0, raw1;
    rc = crumbs_msg_read_u16(reply.data, reply.data_len, 0, &raw0);
    if (rc != 0)
        return rc;
    rc = crumbs_msg_read_u16(reply.data, reply.data_len, 2, &raw1);
    if (rc != 0)
        return rc;
    out->temp_ch0 = (int16_t)raw0;
    out->temp_ch1 = (int16_t)raw1;
    return 0;
}
```

> **Why `crumbs_controller_read` instead of calling `read_fn` directly?**
>
> `crumbs_controller_read` validates the frame length, calls `crumbs_decode_message` (CRC check + struct population), and returns a clean `crumbs_message_t`. Calling `read_fn` directly skips CRC validation and leaves raw bytes in a local buffer, which every caller would have to manage identically. The core function exists precisely so ops headers don't have to repeat this 4-line pattern.

---

## Step 7: Use the ops header on Linux

```c
#include "crumbs_linux.h"
#include "therm_ops.h"

crumbs_context_t ctx;
crumbs_linux_i2c_t lw;
crumbs_linux_init_controller(&ctx, &lw, "/dev/i2c-1", 0);

/* SET: configure sample rate */
therm_send_set_sample_rate(&ctx, THERM_ADDR,
                           crumbs_linux_i2c_write, &lw, 5);

/* GET: read temperatures */
therm_temp_result_t temps;
int rc = therm_get_temp(&ctx, THERM_ADDR,
                        crumbs_linux_i2c_write,
                        crumbs_linux_read,
                        crumbs_linux_delay_us,
                        &lw, &temps);
if (rc == 0) {
    printf("Ch0: %.2f C\n", temps.temp_ch0 / 100.0);
    printf("Ch1: %.2f C\n", temps.temp_ch1 / 100.0);
}

crumbs_linux_close(&lw);
```

---

## Step 8: Use the ops header on Arduino

```cpp
#include "crumbs_arduino.h"
#include "therm_ops.h"

crumbs_context_t ctx;
crumbs_arduino_init_controller(&ctx);

therm_temp_result_t temps;
int rc = therm_get_temp(&ctx, THERM_ADDR,
                        crumbs_arduino_wire_write,
                        crumbs_arduino_read,
                        crumbs_arduino_delay_us,
                        NULL, &temps);
if (rc == 0) {
    Serial.print("Ch0: ");
    Serial.println(temps.temp_ch0 / 100.0);
}
```

The ops header is identical on both platforms. Only the function pointers (`write_fn`, `read_fn`, `delay_fn`) and the `io` context differ.

---

## Reference: payload helper functions

| Helper                                    | Description                    |
| ----------------------------------------- | ------------------------------ |
| `crumbs_msg_add_u8(msg, v)`               | Append 1-byte value to payload |
| `crumbs_msg_add_u16(msg, v)`              | Append 2-byte LE value         |
| `crumbs_msg_add_u32(msg, v)`              | Append 4-byte LE value         |
| `crumbs_msg_read_u8(data, len, off, &v)`  | Read 1 byte at offset          |
| `crumbs_msg_read_u16(data, len, off, &v)` | Read 2 bytes LE at offset      |
| `crumbs_msg_read_u32(data, len, off, &v)` | Read 4 bytes LE at offset      |

Maximum payload is `CRUMBS_MAX_PAYLOAD` bytes (27). All multi-byte values are little-endian.

---

## Reference: platform function pointers

| Symbol                | Linux                              | Arduino                     |
| --------------------- | ---------------------------------- | --------------------------- |
| `crumbs_i2c_write_fn` | `crumbs_linux_i2c_write`           | `crumbs_arduino_wire_write` |
| `crumbs_i2c_read_fn`  | `crumbs_linux_read`                | `crumbs_arduino_read`       |
| `crumbs_delay_fn`     | `crumbs_linux_delay_us`            | `crumbs_arduino_delay_us`   |
| `io` context          | `(void *)&lw` (crumbs_linux_i2c_t) | `NULL` (uses global Wire)   |

---

## Checklist

- [ ] Type ID chosen and documented, does not conflict with existing families
- [ ] All opcodes defined with payload format in comments
- [ ] One `*_send_*` function per SET operation
- [ ] One `*_query_*` function per GET operation (sends SET_REPLY)
- [ ] One `*_result_t` struct per GET operation
- [ ] One `*_get_*` function per GET operation (query + delay + read + parse)
- [ ] All `_get_*` functions return 0 on success, non-zero on error
- [ ] Header guard (`#ifndef / #define / #endif`) in place
- [ ] `#ifdef __cplusplus extern "C"` wrapper present for C++ compatibility
- [ ] Works with both Linux and Arduino function pointer combinations
