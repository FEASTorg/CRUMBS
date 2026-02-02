# Manual Controller

Preconfigured controller for CRUMBS lhwit_family peripherals on Linux I2C.

## Overview

This controller demonstrates proper CRUMBS usage with preconfigured addresses:

**CRUMBS Patterns Demonstrated:**

- Canonical `*_ops.h` helper functions - Protocol-defined command builders
- SET_REPLY query pattern - Two-step query/read for GET operations
- Platform-specific `crumbs_linux_read_message()` - Linux I2C read wrapper
- Configuration pattern - Fixed addresses defined in header

**Application Features:**

- Uses fixed I2C addresses configured in [config.h](config.h)
- Interactive shell for controlling Calculator, LED, Servo, and Display peripherals
- No device discovery required - faster startup
- Standard/default pattern for production use

## Configuration

Device addresses are set in [config.h](config.h):

```c
#define CALCULATOR_ADDR 0x10
#define LED_ADDR        0x20
#define SERVO_ADDR      0x30
```

Modify these values to match your hardware setup.

## Usage

```bash
# Build
cd examples/families_usage/controller_manual
mkdir build && cd build
cmake ..
make

# Run (default /dev/i2c-1)
./controller_manual

# Run with specific I2C device
./controller_manual /dev/i2c-0
```

## Commands

### Calculator

- `calculator add <a> <b>` - Add two numbers
- `calculator sub <a> <b>` - Subtract
- `calculator mul <a> <b>` - Multiply
- `calculator div <a> <b>` - Divide
- `calculator result` - Get last result
- `calculator history` - Show operation history

### LED

- `led set_all <mask>` - Set all LEDs (e.g., 0x0F for all on)
- `led set_one <idx> <state>` - Set single LED
- `led blink <idx> <enable> <period_ms>` - Configure blink
- `led get_state` - Get current LED state

### Servo

- `servo set_pos <idx> <angle>` - Set position (0-180°)
- `servo set_speed <idx> <speed>` - Set speed (0-20)
- `servo sweep <idx> <enable> <min> <max> <step>` - Configure sweep
- `servo get_pos` - Get current positions

## Example Session

```
lhwit> calculator add 42 8
OK: add(42, 8) sent. Use 'calculator result' to get answer.

lhwit> calculator result
Result: 50

lhwit> led set_all 0x0F
OK: LEDs set to 0x0F

lhwit> servo set_pos 0 90
OK: Servo 0 position set to 90°
```

## When to Use This Controller

**Use manual controller when:**

- Device addresses are fixed and known (most common case)
- Production deployments with stable hardware
- Faster startup - no scan delay
- Simpler code for reference implementations
- Following standard patterns from other CRUMBS examples

**Use discovery controller when:**

- Testing with unknown device addresses
- Working with dynamic bus configurations
- Prototyping with multiple device setups
- Need to identify available devices

## Differences from Discovery Controller

The manual controller is **nearly identical** to the discovery controller, with only these differences:

1. **Address initialization**: Uses `config.h` constants instead of 0
2. **No scan command**: Assumes devices are present at configured addresses
3. **No device-found checks**: Commands execute directly without checking if device was discovered

All command implementations are identical between both controllers.

## Requirements

- Linux with I2C support (Raspberry Pi, etc.)
- libi2c-dev installed
- User permissions for I2C access (or sudo)
- Devices configured at addresses specified in config.h
