# Handler Examples

These examples demonstrate the command handler dispatch system in CRUMBS.

## Directory Structure

```
handlers/
├── arduino/                    # Arduino IDE sketches
│   ├── led_peripheral/         # LED array peripheral
│   ├── led_peripheral_minimal/ # Minimal debug version
│   └── servo_peripheral/       # Servo controller peripheral
└── linux/                      # Linux examples
    └── multi_controller/       # Controller for both devices
```

## PlatformIO Examples

For PlatformIO users, see:

- `examples/platformio/handlers/led_peripheral/`
- `examples/platformio/handlers/servo_peripheral/`

These include proper `platformio.ini` configuration with `build_flags` for memory optimization.

## Arduino IDE Limitation

**Important**: Arduino IDE does not support relative includes above the sketch folder.

For Arduino IDE users, command header files (e.g., `led_commands.h`) must be copied into the sketch folder. This is already done for the examples in `handlers/arduino/`.

If you create a new peripheral:

1. Copy the relevant `*_commands.h` file into your sketch folder
2. Include it with `#include "led_commands.h"` (quotes, not angle brackets)

**Recommendation**: Use PlatformIO for a better development experience. It supports:

- `build_flags` for compile-time configuration
- Proper include paths
- Better dependency management

## Memory Optimization

The default `CRUMBS_MAX_HANDLERS` is 16. For memory-constrained devices:

### PlatformIO

Add to `platformio.ini`:

```ini
build_flags = -DCRUMBS_MAX_HANDLERS=8
```

### Arduino IDE

**No direct support.** Options:

1. Edit the library's `crumbs.h` (not recommended, lost on update)
2. Use PlatformIO instead

## Command Definitions

Each peripheral type has a command header defining:

- Type ID (unique per device type)
- Command codes (unique per command within type)
- Controller-side helper functions for sending commands

### LED Commands (`led_commands.h`)

| Command   | Code | Payload                     | Description                |
| --------- | ---- | --------------------------- | -------------------------- |
| SET_ALL   | 0x01 | `[bitmask:u8]`              | Set all LEDs using bitmask |
| SET_ONE   | 0x02 | `[index:u8, state:u8]`      | Set single LED             |
| BLINK     | 0x03 | `[count:u8, delay_10ms:u8]` | Blink all LEDs             |
| GET_STATE | 0x10 | none                        | Request current state      |

### Servo Commands (`servo_commands.h`)

| Command    | Code | Payload                            | Description            |
| ---------- | ---- | ---------------------------------- | ---------------------- |
| SET_ANGLE  | 0x01 | `[channel:u8, angle:u8]`           | Set single servo       |
| SET_BOTH   | 0x02 | `[angle0:u8, angle1:u8]`           | Set both servos        |
| SWEEP      | 0x03 | `[ch:u8, start:u8, end:u8, ms:u8]` | Sweep servo            |
| CENTER_ALL | 0x04 | none                               | Center all to 90°      |
| GET_ANGLES | 0x10 | none                               | Request current angles |

## Hardware Setup

### LED Peripheral (Address: 0x08)

- Arduino Nano (or compatible)
- 4 LEDs on pins D4, D5, D6, D7
- Connect SDA (A4) and SCL (A5) to I2C bus
- Connect GND

### Servo Peripheral (Address: 0x09)

- Arduino Nano (or compatible)
- 2 SG90 servos on pins D9, D10
- Connect SDA (A4) and SCL (A5) to I2C bus
- Connect GND
- External 5V power for servos recommended

### Controller

- Raspberry Pi or other Linux SBC
- Connect SDA, SCL, GND to I2C bus
- Enable I2C: `sudo raspi-config` → Interface Options → I2C
