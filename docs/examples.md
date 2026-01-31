# Examples

## Controller Example

Send commands via serial interface and request data from peripherals. The examples in `examples/arduino/` and `examples/platformio/` show both controller and peripheral sketches using the C core API (see `crumbs_arduino.h`).

### Usage

1. Upload to Arduino
2. Open Serial Monitor (115200 baud)
3. Send: `address,type_id,opcode,byte0,byte1,...` (comma-separated bytes)
4. Request: `request=address`

### Example Commands

```cpp
8,1,1,0,0,128,63,0,0,0,0     // Send to address 8 (first 4 bytes = 1.0f in little-endian)
request=8                    // Request data from address 8
```

## Peripheral Example

Respond to controller commands and data requests. The C core uses callbacks with signatures `on_message(ctx, const crumbs_message_t*)` and `on_request(ctx, crumbs_message_t*)`.

### Key Code Patterns (C API — Arduino HAL)

```cpp
// Message callback from the core
void on_message(crumbs_context_t *ctx, const crumbs_message_t *m) {
    switch (m->opcode) {
        case 0: // Data request (handled in on_request instead)
            break;
        case 1: // Set parameters
            // read bytes from m->data[0..m->data_len-1] and apply
            // example: decode a float from bytes
            if (m->data_len >= sizeof(float)) {
                float val;
                memcpy(&val, m->data, sizeof(float));
            }
            break;
    }
}

// Build reply for onRequest — the framework will encode and Wire.write the result
void on_request(crumbs_context_t *ctx, crumbs_message_t *reply) {
    reply->type_id = 1;
    reply->opcode = 0;
    float val = 42.0f;
    reply->data_len = sizeof(float);
    memcpy(reply->data, &val, sizeof(float));
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
}
```

## Handler-Based Peripheral (Alternative Pattern)

Instead of using a switch statement inside `on_message`, you can register individual handler functions for each command type. This pattern is cleaner when you have many commands.

### Arduino Peripheral with Handlers

```cpp
#include <crumbs.h>
#include <crumbs_arduino.h>

#define CMD_LED_ON  0x01
#define CMD_LED_OFF 0x02
#define CMD_BLINK   0x03

static crumbs_context_t ctx;

void handleLedOn(crumbs_context_t *ctx, uint8_t cmd,
                 const uint8_t *data, uint8_t len, void *user) {
    digitalWrite(LED_BUILTIN, HIGH);
}

void handleLedOff(crumbs_context_t *ctx, uint8_t cmd,
                  const uint8_t *data, uint8_t len, void *user) {
    digitalWrite(LED_BUILTIN, LOW);
}

void handleBlink(crumbs_context_t *ctx, uint8_t cmd,
                 const uint8_t *data, uint8_t len, void *user) {
    if (len < 2) return;
    uint8_t count = data[0];
    uint8_t delayMs = data[1] * 10;
    for (uint8_t i = 0; i < count; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_BUILTIN, LOW);
        delay(delayMs);
    }
}

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    crumbs_arduino_init_peripheral(&ctx, 0x08);

    // Register per-command handlers
    crumbs_register_handler(&ctx, CMD_LED_ON,  handleLedOn,  NULL);
    crumbs_register_handler(&ctx, CMD_LED_OFF, handleLedOff, NULL);
    crumbs_register_handler(&ctx, CMD_BLINK,   handleBlink,  NULL);
}

void loop() { }
```

### Linux Controller for Handler Peripheral

```c
// Send LED ON command
crumbs_message_t msg = {0};
msg.type_id = 1;
msg.opcode = 0x01;  // CMD_LED_ON
msg.data_len = 0;
crumbs_controller_send(&ctx, 0x08, &msg, crumbs_linux_i2c_write, &lw);

// Send BLINK command: 5 blinks, 200ms delay
msg.opcode = 0x03;  // CMD_BLINK
msg.data_len = 2;
msg.data[0] = 5;   // count
msg.data[1] = 20;  // delay = 20 * 10ms = 200ms
crumbs_controller_send(&ctx, 0x08, &msg, crumbs_linux_i2c_write, &lw);
```

See `examples/arduino/handler_peripheral_led/` and `examples/linux/multi_handler_controller/` for complete working examples.

## Message Helpers Pattern

The `crumbs_message_helpers.h` header provides type-safe payload building and reading. This pattern is cleaner than manual byte manipulation:

### Controller with Message Helpers

```c
#include <crumbs_message_helpers.h>

// Define command header (see examples/common/ for full pattern)
#define SERVO_TYPE_ID    0x02
#define SERVO_CMD_ANGLE  0x01

crumbs_message_t msg;
crumbs_msg_init(&msg);
msg.type_id = SERVO_TYPE_ID;
msg.opcode = SERVO_CMD_ANGLE;
crumbs_msg_add_u8(&msg, servo_index);    // Which servo
crumbs_msg_add_u16(&msg, 1500);          // Pulse width in μs

crumbs_controller_send(&ctx, 0x10, &msg, write_fn, write_ctx);
```

### Peripheral Handler with Message Readers

```c
void handle_servo_angle(crumbs_context_t *ctx, uint8_t cmd,
                        const uint8_t *data, uint8_t len, void *user) {
    size_t off = 0;
    uint8_t index;
    uint16_t pulse;

    if (crumbs_msg_read_u8(data, len, &off, &index) < 0) return;
    if (crumbs_msg_read_u16(data, len, &off, &pulse) < 0) return;

    servo_set_pulse(index, pulse);
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, 0x10);
    crumbs_register_handler(&ctx, SERVO_CMD_ANGLE, handle_servo_angle, NULL);
}
```

See the handler examples for complete working code:

- `examples/arduino/handler_peripheral_led/` — LED strip control with RGB values
- `examples/arduino/handler_peripheral_servo/` — Servo control with pulse widths
- `examples/linux/multi_handler_controller/` — Linux controller using multiple device command headers
- `examples/linux/interactive_controller/` — Interactive CLI controller for LED and servo peripherals

See [Message Helpers](message-helpers.md) for complete API documentation.

## Common Patterns

### Device Proxy Pattern (Controller-Side)

When sending many commands to the same device, you can bundle the context, address, and I/O function into a struct to reduce repetition:

```c
// Bundle device parameters (user-side pattern, not library API)
typedef struct {
    crumbs_context_t *ctx;
    uint8_t addr;
    crumbs_i2c_write_fn write_fn;
    void *io;
} led_device_t;

// Initialize once
led_device_t led = { &ctx, 0x08, crumbs_arduino_wire_write, NULL };

// Shorter sender wrapper
static inline int led_set_all(led_device_t *dev, uint8_t bitmask) {
    crumbs_message_t msg;
    crumbs_msg_init(&msg);
    msg.type_id = LED_TYPE_ID;
    msg.opcode = LED_CMD_SET_ALL;
    crumbs_msg_add_u8(&msg, bitmask);
    return crumbs_controller_send(dev->ctx, dev->addr, &msg, dev->write_fn, dev->io);
}

// Clean usage
led_set_all(&led, 0x0F);
```

This is an optional convenience pattern. The library's explicit parameter passing remains the primitive for maximum flexibility.

### Multiple Devices

```cpp
uint8_t addresses[] = {0x08, 0x09, 0x0A};
for (int i = 0; i < 3; i++) {
    crumbs_controller_send(&ctx, addresses[i], &msg, crumbs_arduino_wire_write, NULL);
    delay(10);
}
```

### Data Requests

On Arduino, use `Wire.requestFrom()` and the core API helper to decode; e.g. read bytes and call `crumbs_decode_message()`.

On Linux, the HAL helper `crumbs_linux_read_message()` wraps the low-level reads and decoding for you.

### CRC Validation

Use `crumbs_decode_message(buffer, bytesRead, &out, ctx)` which returns 0 on success, -1 if too small and -2 on CRC mismatch.

### I2C Scanning

```cpp
for (uint8_t addr = 8; addr < 120; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
        Serial.print("Found device at 0x");
        Serial.println(addr, HEX);
    }
}
```

For protocol-aware discovery (find devices that actually speak CRUMBS) use the core helper `crumbs_controller_scan_for_crumbs()` which performs a read-and-decode probe. See the Getting Started guide for short Arduino and Linux examples.

## Native C example (CMake + static link)

There is a minimal native C example that demonstrates using CRUMBS as a compiled library and linking a small program against the `crumbs` static target. The example is located at `examples/linux/simple_native_controller`.

Build the example in-tree (recommended for local development):

```bash
cmake -S examples/linux/simple_native_controller -B examples/linux/simple_native_controller/build -DCRUMBS_BUILD_IN_TREE=ON
cmake --build examples/linux/simple_native_controller/build --config Release
./examples/linux/simple_native_controller/build/crumbs_native_example
```

If you have installed CRUMBS (and supplied CMake config files to your install prefix) you can instead build the example against an installed package by turning off `CRUMBS_BUILD_IN_TREE` and setting `CMAKE_PREFIX_PATH` accordingly.

When CRUMBS is installed and its CMake package files are available, out-of-tree builds can use `find_package`:

```cmake
find_package(crumbs CONFIG REQUIRED)
add_executable(myprog main.c)
target_link_libraries(myprog PRIVATE crumbs::crumbs)
```

## PlatformIO examples (Arduino Nano)

There are several PlatformIO examples that target the Arduino Nano:

- `examples/platformio/simple_controller/` — controller sketch that reads serial CSV commands and sends them to a peripheral.
- `examples/platformio/simple_peripheral/` — peripheral sketch (address 0x08) that prints received commands and returns sample replies.
- `examples/platformio/handler_peripheral_led/` — LED peripheral using handler dispatch pattern.
- `examples/platformio/handler_peripheral_servo/` — servo peripheral using handler dispatch pattern.

Build with PlatformIO CLI inside the example directories:

```bash
cd examples/platformio/simple_controller
pio run -e nanoatmega328new

cd ../simple_peripheral
pio run -e nanoatmega328new
```
