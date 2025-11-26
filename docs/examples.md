# Examples

## Controller Example

Send commands via serial interface and request data from peripherals. The examples in `examples/arduino` show both controller and peripheral sketches using the C core API (see `crumbs_arduino.h`).

### Usage

1. Upload to Arduino
2. Open Serial Monitor (115200 baud)
3. Send: `address,type_id,command_type,data0,data1,data2,data3,data4,data5,data6`
4. Request: `request=address`

### Example Commands

```cpp
8,1,1,75.0,1.0,0.0,65.0,2.0,7.0,3.14     // Send to address 8
request=8                                 // Request data from address 8
```

## Peripheral Example

Respond to controller commands and data requests. The C core uses callbacks with signatures `on_message(ctx, const crumbs_message_t*)` and `on_request(ctx, crumbs_message_t*)`.

### Key Code Patterns (C API — Arduino HAL)

```cpp
// Message callback from the core
void on_message(crumbs_context_t *ctx, const crumbs_message_t *m) {
    switch (m->command_type) {
        case 0: // Data request (handled in on_request instead)
            break;
        case 1: // Set parameters
            // read floats from m->data[] and apply
            break;
    }
}

// Build reply for onRequest — the framework will encode and Wire.write the result
void on_request(crumbs_context_t *ctx, crumbs_message_t *reply) {
    reply->type_id = 1;
    reply->command_type = 0;
    reply->data[0] = 42.0f;
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
}
```

## Common Patterns

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

There is a minimal native C example that demonstrates using CRUMBS as a compiled library and linking a small program against the `crumbs` static target. The example is located at `examples/native/controller`.

Build the example in-tree (recommended for local development):

```bash
cmake -S examples/native/controller -B examples/native/controller/build -DCRUMBS_BUILD_IN_TREE=ON
cmake --build examples/native/controller/build --config Release
./examples/native/controller/build/crumbs_native_example
```

If you have installed CRUMBS (and supplied CMake config files to your install prefix) you can instead build the example against an installed package by turning off `CRUMBS_BUILD_IN_TREE` and setting `CMAKE_PREFIX_PATH` accordingly.

When CRUMBS is installed and its CMake package files are available, out-of-tree builds can use `find_package`:

```cmake
find_package(crumbs CONFIG REQUIRED)
add_executable(myprog main.c)
target_link_libraries(myprog PRIVATE crumbs::crumbs)
```

## PlatformIO examples (Arduino Nano)

There are two small PlatformIO examples that target the Arduino Nano:

- `examples/platformio/controller` — controller sketch that reads serial CSV commands (addr,type_id,command_type,data...) and sends them to a peripheral.
- `examples/platformio/peripheral` — peripheral sketch (address 0x08) that prints received commands to Serial and returns a sample reply for requests.

Build with PlatformIO CLI inside the example directories:

```bash
cd examples/platformio/controller
pio run -e nanoatmega328

cd ../peripheral
pio run -e nanoatmega328
```
