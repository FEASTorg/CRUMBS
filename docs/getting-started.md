# Getting Started

## Installation

1. Download or clone the CRUMBS repository.
2. Place the folder in your Arduino `libraries` directory (or use the platform's library manager).
3. Include the headers your target requires:

- Arduino sketches: `#include <crumbs_arduino.h>` (this pulls in `crumbs.h`)
  -- Linux projects: `#include "crumbs.h"` and `#include "crumbs_linux.h"` as needed

1. No external dependencies required.

### Native C (CMake) installation

You can build and install CRUMBS as a CMake library and then use `find_package(crumbs CONFIG)` from other projects.

In the source tree:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
cmake --install build --prefix /usr/local
```

After installation you can use CRUMBS in other CMake projects with:

```cmake
find_package(crumbs CONFIG REQUIRED)
add_executable(myprog main.c)
target_link_libraries(myprog PRIVATE crumbs::crumbs)
```

Tips

- For local development prefer the in-tree example usage (`examples/linux/simple_native_controller/`) which will link the in-repo `crumbs` target via `add_subdirectory`.
- When packaging for distribution ensure you install the exported cmake files (the top-level CMake provided export/install rules place crumbs targets under lib/cmake/crumbs) so `find_package(crumbs CONFIG)` can locate them via CMAKE_PREFIX_PATH or an installed system prefix.

Linux HAL dependency note

- If you build CRUMBS with the Linux HAL enabled (CRUMBS_ENABLE_LINUX_HAL=ON) the installed CMake package will require the `linux_wire` package at consumption time (the exported target `crumbs::crumbs` links against `linux_wire::linux_wire`). Consumers should ensure `linux-wire` is available in their CMAKE_PREFIX_PATH or a system-installed location so the generated `crumbsConfig.cmake` can satisfy the dependency via `find_dependency(linux_wire CONFIG REQUIRED)`.

## Basic Usage

### Controller (Arduino — C API)

```cpp
#include <crumbs_arduino.h>

crumbs_context_t ctx;

void setup() {
    // Initialize the Arduino HAL for controller mode
    crumbs_arduino_init_controller(&ctx);

    crumbs_message_t m = {};
    m.type_id = 1;
    m.command_type = 1;

    // Variable-length payload: send a float as 4 bytes
    float temp = 25.5f;
    m.data_len = sizeof(float);
    memcpy(m.data, &temp, sizeof(float));

    // Send to target address 0x08 using the Wire HAL via crumbs_controller_send
    crumbs_controller_send(&ctx, 0x08, &m, crumbs_arduino_wire_write, NULL);
}
```

### Peripheral (Arduino — C API)

```cpp
#include <crumbs_arduino.h>

crumbs_context_t ctx;

// Message receive callback
void on_message(crumbs_context_t *ctx, const crumbs_message_t *m) {
    // process the message: m->data_len bytes in m->data[]
    if (m->data_len >= sizeof(float)) {
        float val;
        memcpy(&val, m->data, sizeof(float));
        // use val...
    }
}

// Request callback — fill reply message
void on_request(crumbs_context_t *ctx, crumbs_message_t *reply) {
    reply->type_id = 1;
    reply->command_type = 0;
    float val = 42.0f;
    reply->data_len = sizeof(float);
    memcpy(reply->data, &val, sizeof(float));
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
}
```

> **Byte Order Note:** Integer helpers (`crumbs_msg_add_u16()`, etc.) use little-endian encoding for portability. However, `crumbs_msg_add_float()` uses **native byte order** (`memcpy`), which is not portable across different architectures. If your controller and peripheral use different CPU architectures (e.g., ARM and x86), you should serialize floats manually or use fixed-point integers instead.

## Hardware Setup

- Connect I2C (SDA/SCL) with 4.7kΩ pull-ups
- Use addresses 0x08-0x77 for peripherals
- Set I2C clock to 100kHz (default)

## Next Steps

Once basic communication is working:

1. **Handler dispatch**: Register per-command handlers instead of switch statements — see [API Reference](api-reference.md#command-handler-dispatch)
2. **Message helpers**: Use `crumbs_msg.h` for type-safe payload building — see [Message Helpers](message-helpers.md)
3. **Command headers**: Create reusable command definitions — see `examples/common/`
4. **Memory optimization**: Reduce handler table size with `CRUMBS_MAX_HANDLERS` — see below

### Quick Handler Dispatch Example

Instead of a large switch statement, register handlers per command:

```cpp
// Peripheral with handler dispatch (Arduino)
#include <crumbs_arduino.h>

crumbs_context_t ctx;

void handle_led(crumbs_context_t *c, uint8_t cmd, const uint8_t *data, uint8_t len, void *user) {
    if (len >= 1) digitalWrite(LED_BUILTIN, data[0] ? HIGH : LOW);
}

void handle_servo(crumbs_context_t *c, uint8_t cmd, const uint8_t *data, uint8_t len, void *user) {
    size_t off = 0;
    uint8_t index;
    uint16_t pulse;
    if (crumbs_msg_read_u8(data, len, &off, &index) == 0 &&
        crumbs_msg_read_u16(data, len, &off, &pulse) == 0) {
        // set_servo(index, pulse);
    }
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_register_handler(&ctx, 0x01, handle_led, NULL);   // command 0x01 → LED
    crumbs_register_handler(&ctx, 0x02, handle_servo, NULL); // command 0x02 → servo
}

void loop() {} // Wire callbacks handle everything
```

For complete examples, see `examples/arduino/handler_peripheral_led/` and `examples/arduino/handler_peripheral_servo/`.

## Memory Optimization

The default handler table (256 entries) uses ~1KB RAM on AVR. For memory-constrained devices, reduce it:

```c
#define CRUMBS_MAX_HANDLERS 8  // Before including crumbs.h
#include <crumbs.h>
```

This reduces handler memory from ~1KB to ~33 bytes. See [API Reference](api-reference.md#memory-optimization-crumbs_max_handlers) for details.

## Debug & Troubleshooting

Enable debug: `#define CRUMBS_DEBUG` before include

**Note**: Only one CRUMBS instance allowed per Arduino (singleton pattern)

Common issues:

- **No response**: Check wiring, addresses, pull-ups
- **Data corruption**: Verify timing, use delays between operations
- **Address conflicts**: Use I2C scanner to verify addresses

### Scanning for CRUMBS devices

The HALs expose a generic I²C scanner (address ACK probing) and the CRUMBS core provides a CRUMBS-aware scanner that attempts to read and decode a CRUMBS frame from each address. Use the CRUMBS scanner when you want to discover devices that actually speak the CRUMBS protocol.

**Scan modes:**

- **Non-strict** (default): Attempts reads, may send a probe write if needed. More likely to detect devices but slightly more intrusive.
- **Strict**: Read-only checks. Safer for buses with sensitive devices but may miss some peripherals.

Arduino example (Serial):

```cpp
// Use crumbs_controller_scan_for_crumbs from the core
// flags=0 for non-strict, flags=CRUMBS_SCAN_STRICT for strict mode
uint8_t found[32];
int n = crumbs_controller_scan_for_crumbs(&ctx, 0x03, 0x77, 0, crumbs_arduino_wire_write, crumbs_arduino_read, &Wire, found, sizeof(found), 50000);
```

Linux example (CLI):

```bash
# Non-strict mode (default)
./crumbs_simple_linux_controller scan

# Strict mode (read-only probes)
./crumbs_simple_linux_controller scan strict
```
