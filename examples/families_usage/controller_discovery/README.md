# Discovery Controller

Auto-discovery controller for CRUMBS lhwit_family peripherals on Linux I2C.

## Overview

This controller demonstrates proper CRUMBS usage with auto-discovery:

**CRUMBS Patterns Demonstrated:**
- `crumbs_controller_scan_for_crumbs_with_types()` - Type-aware bus scanning
- Canonical `*_ops.h` helper functions - Protocol-defined command builders
- SET_REPLY query pattern - Two-step query/read for GET operations
- Platform-specific `crumbs_linux_read_message()` - Linux I2C read wrapper

**Application Features:**
- Automatically finds Calculator (0x03), LED (0x01), and Servo (0x02) devices
- Interactive shell for controlling discovered peripherals
- Device-found checks before executing commands

## Usage

```bash
# Build
cd examples/families_usage/controller_discovery
mkdir build && cd build
cmake ..
make

# Run (default /dev/i2c-1)
./controller_discovery

# Run with specific I2C device
./controller_discovery /dev/i2c-0
```

## Commands

### Discovery
- `scan` - Scan I2C bus for CRUMBS devices

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
lhwit> scan
Found 3 CRUMBS device(s):
  [0] Address 0x10, Type 0x03 (Calculator)
  [1] Address 0x20, Type 0x01 (LED)
  [2] Address 0x30, Type 0x02 (Servo)

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

**Use discovery controller when:**
- Testing with unknown device addresses
- Working with dynamic bus configurations
- Prototyping with multiple device setups
- Need to identify available devices

**Use manual controller when:**
- Addresses are fixed and known
- Production deployments
- Faster startup (no scan required)
- Simpler code for reference

## Requirements

- Linux with I2C support (Raspberry Pi, etc.)
- libi2c-dev installed
- User permissions for I2C access (or sudo)
