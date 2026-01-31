# Handler Dispatch Guide

This guide covers the CRUMBS handler dispatch system — an alternative to switch statements for processing incoming commands on peripheral devices.

## Overview

Instead of writing large switch statements in your `on_message` callback, you can register per-command handler functions. CRUMBS will automatically dispatch to the correct handler when a message arrives.

**Benefits:**

- Cleaner code organization (one function per command)
- Easy to add/remove commands without modifying a central switch
- Supports per-handler user data
- Memory-efficient on AVR when tuned with `CRUMBS_MAX_HANDLERS`

## Handler Function Signature

All handlers use this signature:

```c
typedef void (*crumbs_handler_fn)(
    crumbs_context_t *ctx,      // The CRUMBS context
    uint8_t opcode,       // The command that triggered this handler
    const uint8_t *data,        // Payload bytes (may be NULL if len==0)
    uint8_t data_len,           // Payload length (0–27)
    void *user_data             // Opaque pointer registered with handler
);
```

## Registering Handlers

Use `crumbs_register_handler()` to associate a command with a function:

```c
int crumbs_register_handler(crumbs_context_t *ctx,
                            uint8_t opcode,
                            crumbs_handler_fn fn,
                            void *user_data);
```

**Returns:** `0` on success, `-1` if context is NULL or handler table is full.

**Example:**

```c
crumbs_register_handler(&ctx, 0x01, handle_led_set, NULL);
crumbs_register_handler(&ctx, 0x02, handle_led_blink, NULL);
crumbs_register_handler(&ctx, 0x10, handle_led_query, &led_state);
```

## Unregistering Handlers

Remove a handler with:

```c
int crumbs_unregister_handler(crumbs_context_t *ctx, uint8_t opcode);
```

Or call `crumbs_register_handler()` with `fn = NULL`.

## Complete Peripheral Example

```cpp
#include <crumbs.h>
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

#define I2C_ADDRESS 0x08

static crumbs_context_t ctx;
static uint8_t led_state = 0;

// Handler: Set all LEDs via bitmask
void handle_set_all(crumbs_context_t *c, uint8_t cmd,
                    const uint8_t *data, uint8_t len, void *user) {
    (void)c; (void)cmd; (void)user;

    uint8_t bitmask;
    if (crumbs_msg_read_u8(data, len, 0, &bitmask) == 0) {
        led_state = bitmask;
        // Apply to hardware...
    }
}

// Handler: Set single LED
void handle_set_one(crumbs_context_t *c, uint8_t cmd,
                    const uint8_t *data, uint8_t len, void *user) {
    (void)c; (void)cmd; (void)user;

    uint8_t index, state;
    if (crumbs_msg_read_u8(data, len, 0, &index) == 0 &&
        crumbs_msg_read_u8(data, len, 1, &state) == 0) {
        if (state) led_state |= (1 << index);
        else led_state &= ~(1 << index);
        // Apply to hardware...
    }
}

// Handler: Respond to state query
void on_request(crumbs_context_t *ctx, crumbs_message_t *reply) {
    crumbs_msg_init(reply, 0x01, 0x10);  // type=LED, opcode=GET_STATE response
    crumbs_msg_add_u8(reply, led_state);
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, I2C_ADDRESS);

    // Register handlers (no on_message callback needed)
    crumbs_register_handler(&ctx, 0x01, handle_set_all, NULL);
    crumbs_register_handler(&ctx, 0x02, handle_set_one, NULL);

    // Register request callback for I2C reads
    crumbs_set_callbacks(&ctx, NULL, on_request, NULL);
}

void loop() {
    // Wire callbacks handle everything
}
```

## Using Message Helpers

The `crumbs_message_helpers.h` helpers provide type-safe payload parsing:

```c
// Reading values (peripheral handler)
uint8_t channel;
uint16_t value;

if (crumbs_msg_read_u8(data, len, 0, &channel) < 0) return;
if (crumbs_msg_read_u16(data, len, 1, &value) < 0) return;
// offset 0 = channel (1 byte), offset 1 = value (2 bytes)
```

Available read functions:

| Function                  | Type     | Size    |
| ------------------------- | -------- | ------- |
| `crumbs_msg_read_u8()`    | uint8_t  | 1 byte  |
| `crumbs_msg_read_u16()`   | uint16_t | 2 bytes |
| `crumbs_msg_read_u32()`   | uint32_t | 4 bytes |
| `crumbs_msg_read_i8()`    | int8_t   | 1 byte  |
| `crumbs_msg_read_i16()`   | int16_t  | 2 bytes |
| `crumbs_msg_read_i32()`   | int32_t  | 4 bytes |
| `crumbs_msg_read_float()` | float    | 4 bytes |
| `crumbs_msg_read_bytes()` | raw      | n bytes |

All functions return `0` on success, `-1` on bounds overflow.

## Command Header Pattern

For reusable command definitions, create a shared header:

```c
// led_commands.h — shared by controller and peripheral

#define LED_TYPE_ID       0x01

#define LED_CMD_SET_ALL   0x01  // Payload: [bitmask:u8]
#define LED_CMD_SET_ONE   0x02  // Payload: [index:u8, state:u8]
#define LED_CMD_BLINK     0x03  // Payload: [count:u8, delay_10ms:u8]
#define LED_CMD_GET_STATE 0x10  // Payload: none (reply has state)
```

See `examples/common/led_commands.h` and `examples/common/servo_commands.h` for complete examples with controller-side sender functions.

## Memory Optimization

The handler table uses `CRUMBS_MAX_HANDLERS` slots (default: 16). On AVR, each slot uses ~4 bytes.

For memory-constrained devices:

```ini
# platformio.ini
build_flags = -DCRUMBS_MAX_HANDLERS=8
```

**Important:** Define `CRUMBS_MAX_HANDLERS` in build flags, not in your sketch. Arduino precompiles the library separately, so sketch-level defines won't affect it.

To disable handler dispatch entirely (use callbacks only):

```ini
build_flags = -DCRUMBS_MAX_HANDLERS=0
```

## Dispatch Order

When a message arrives:

1. `on_message` callback is invoked first (if registered)
2. Handler dispatch searches for a matching `opcode`
3. If found, the handler is invoked with payload data

Both mechanisms can coexist — use `on_message` for logging/validation, handlers for command processing.

## Error Handling

Handlers should validate payload length before reading:

```c
void handle_servo(crumbs_context_t *c, uint8_t cmd,
                  const uint8_t *data, uint8_t len, void *user) {
    uint8_t channel, angle;

    // These return -1 if there aren't enough bytes
    if (crumbs_msg_read_u8(data, len, 0, &channel) < 0) {
        // Log error, return early
        return;
    }
    if (crumbs_msg_read_u8(data, len, 1, &angle) < 0) {
        return;
    }

    // Process valid data
    set_servo(channel, angle);
}
```

## See Also

- [API Reference — Handler Dispatch](api-reference.md#command-handler-dispatch)
- [Message Helpers](message-helpers.md)
- [Getting Started](getting-started.md)
- Example: `examples/arduino/handler_peripheral_led/`
- Example: `examples/arduino/handler_peripheral_servo/`
