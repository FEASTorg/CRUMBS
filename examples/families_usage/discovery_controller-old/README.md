# LHWIT Discovery Controller

Auto-discovery interactive controller for the LHWIT (Low Hardware Implementation Test) device family. This controller demonstrates how to scan the I²C bus for CRUMBS devices, identify them by type, and control them through an interactive command-line interface.

## Overview

The discovery controller showcases the **auto-discovery pattern** where device addresses are determined at runtime by scanning the I²C bus and identifying devices by their type ID. This is ideal for:

- **Development and testing**: Quickly discover and interact with peripherals without hardcoding addresses
- **Dynamic environments**: Handle devices that might be at different addresses
- **Learning**: Understand how CRUMBS type identification works

**Contrast with manual_controller**: The manual controller uses hardcoded addresses from a config file, which is faster but less flexible. Choose based on your use case.

## Supported Devices

This controller works with the lhwit_family peripherals:

| Device Type          | Type ID | Typical Address | Description                       |
| -------------------- | ------- | --------------- | --------------------------------- |
| **Calculator**       | 0x03    | 0x10            | 32-bit integer math with history  |
| **LED Array**        | 0x01    | 0x20            | 4-LED control (D4-D7)             |
| **Servo Controller** | 0x02    | 0x30            | 2-servo position control (D9-D10) |

## Hardware Setup

### Required Components

- **1x Linux SBC** (Raspberry Pi, Orange Pi, etc.) - The controller
- **3x Arduino Nano boards** - Running the lhwit_family peripherals
- **4x LEDs + resistors** (220Ω) - For LED peripheral
- **2x Servo motors** (SG90 or similar) - For servo peripheral
- **1x Breadboard** - For connections
- **Jumper wires** - For I²C bus
- **External 5V power supply** - For servos (REQUIRED)

### Wiring

All three peripherals connect to the same I²C bus:

```
┌─────────────────┐
│   Linux SBC     │
│   (Controller)  │
│                 │
│  SDA ───────────┼─────┬─────┬─────┐
│  SCL ───────────┼─────┼─────┼─────┤
│  GND ───────────┼─────┼─────┼─────┤
└─────────────────┘     │     │     │
                        │     │     │
              ┌─────────┤     │     │
              │  Nano   │     │     │
              │ Calc    │     │     │
              │ (0x10)  │     │     │
              └─────────┘     │     │
                              │     │
                    ┌─────────┤     │
                    │  Nano   │     │
                    │  LED    │     │
                    │ (0x20)  │     │
                    └─────────┘     │
                                    │
                          ┌─────────┤
                          │  Nano   │
                          │ Servo   │
                          │ (0x30)  │
                          └─────────┘
```

**I²C Connections** (for each Arduino Nano):

- SDA → A4
- SCL → A5
- GND → GND

**Servo Power Warning**: Do NOT power servos from Arduino Nano's 5V pin. Use an external 5V power supply with sufficient current capacity (at least 1A per servo).

## Building

### Prerequisites

- CMake 3.13 or newer
- C compiler (GCC/Clang)
- Linux with I²C support (`i2c-dev` kernel module)
- CRUMBS library with linux-wire support

### Compilation

```bash
# From this directory
mkdir -p build && cd build
cmake ..
make

# The executable will be: ./lhwit_discovery_controller
```

### Troubleshooting Build Issues

**Error: "crumbs.h not found"**

- The CMakeLists.txt assumes CRUMBS_PATH is 3 levels up
- Set manually: `cmake .. -DCRUMBS_PATH=/path/to/CRUMBS`

**Error: "linux-wire not found"**

- CRUMBS must be built with `CRUMBS_ENABLE_LINUX_HAL=ON`
- Rebuild CRUMBS with Linux support enabled

## Usage

### Basic Workflow

1. **Flash peripherals**: Upload firmware to all three Arduino Nanos using PlatformIO
2. **Connect hardware**: Wire the I²C bus as shown above
3. **Run controller**: Execute the discovery controller binary
4. **Scan for devices**: Use the `scan` command to find peripherals
5. **Issue commands**: Control devices using interactive commands

### Running the Controller

```bash
# Use default I²C device (/dev/i2c-1)
./lhwit_discovery_controller

# Or specify custom I²C device
./lhwit_discovery_controller /dev/i2c-0
```

### I²C Permissions

If you get a permission denied error:

```bash
# Temporary fix (until reboot)
sudo chmod 666 /dev/i2c-1

# Permanent fix (add user to i2c group)
sudo usermod -a -G i2c $USER
# Log out and back in for changes to take effect
```

## Interactive Commands

### General Commands

```
help                    Show all available commands
scan                    Discover devices on I²C bus
quit, exit              Exit the controller
```

### Scanning for Devices

The `scan` command searches the I²C bus for CRUMBS devices and identifies them by type:

```
lhwit> scan
Scanning I2C bus for CRUMBS devices (0x08-0x77)...
  Found 3 device(s):
    0x10: Type 0x03 - Calculator
    0x20: Type 0x01 - LED Array
    0x30: Type 0x02 - Servo Controller

Device addresses stored. You can now use device commands.
```

**What happens during scan**:

1. Controller sends PING messages to addresses 0x08-0x77
2. Devices respond with their type ID
3. Controller stores addresses for each discovered type
4. You can now use device-specific commands

### Calculator Commands

**Arithmetic operations** (results stored on peripheral):

```
calculator add <a> <b>        Add two numbers
calculator sub <a> <b>        Subtract (a - b)
calculator mul <a> <b>        Multiply two numbers
calculator div <a> <b>        Divide (a / b)
```

**Query results**:

```
calculator result             Get last calculation result
calculator history            Get all history entries (up to 12)
```

**Example session**:

```
lhwit> calculator add 100 200
OK: add(100, 200) command sent
    Use 'calculator result' to get the answer

lhwit> calculator result
Last result: 300

lhwit> calculator mul 15 20
OK: mul(15, 20) command sent
    Use 'calculator result' to get the answer

lhwit> calculator result
Last result: 300

lhwit> calculator history
History: 2 entries (write position: 2)
  [0] ADD (100, 200) = 300
  [1] MUL (15, 20) = 300
```

### LED Commands

**Control LEDs**:

```
led set_all <mask>            Set all 4 LEDs at once (bitmask)
led set_one <idx> <0|1>       Set single LED on/off (idx 0-3)
led blink <idx> <en> <ms>     Configure blinking (en: 0=off, 1=on)
```

**Query state**:

```
led get_state                 Get current LED states
```

**Example session**:

```
lhwit> led set_all 0x0F
OK: All LEDs set to 0x0F

lhwit> led get_state
LED state: 0x0F (1111)
  LED 0: ON, LED 1: ON, LED 2: ON, LED 3: ON

lhwit> led set_one 2 0
OK: LED 2 set to OFF

lhwit> led get_state
LED state: 0x0B (1011)
  LED 0: ON, LED 1: ON, LED 2: OFF, LED 3: ON

lhwit> led blink 0 1 500
OK: LED 0 blinking with 500 ms period
```

**Bitmask reference**:

- Bit 0 (0x01) = LED 0
- Bit 1 (0x02) = LED 1
- Bit 2 (0x04) = LED 2
- Bit 3 (0x08) = LED 3
- Example: 0x0F = all ON, 0x05 = LEDs 0 and 2 ON

### Servo Commands

**Control servos**:

```
servo set_pos <idx> <angle>       Set position (idx 0-1, angle 0-180°)
servo set_speed <idx> <speed>     Set movement speed (0=instant, 1-20=slow)
servo sweep <idx> <en> <min> <max> <step>   Configure sweep pattern
```

**Query state**:

```
servo get_pos                     Get current positions
```

**Example session**:

```
lhwit> servo set_pos 0 90
OK: Servo 0 set to 90 degrees

lhwit> servo set_pos 1 45
OK: Servo 1 set to 45 degrees

lhwit> servo get_pos
Servo positions: Servo 0 = 90°, Servo 1 = 45°

lhwit> servo set_speed 0 5
OK: Servo 0 speed set to 5 (0=instant, higher=slower)

lhwit> servo sweep 0 1 0 180 10
OK: Servo 0 sweeping from 0 to 180 degrees (step 10)

lhwit> servo sweep 0 0 0 0 0
OK: Servo 0 sweep disabled
```

**Speed values**:

- 0 = Instant movement (no speed limiting)
- 1-20 = Degrees per update cycle (higher = slower movement)

## Troubleshooting

### No Devices Found During Scan

**Check:**

1. All peripherals are powered on
2. I²C wiring is correct (SDA, SCL, GND)
3. Peripherals have been flashed with the correct firmware
4. No address conflicts (each peripheral must have unique address)
5. Pull-up resistors on SDA/SCL lines (usually built-in, but check if issues persist)

### Command Returns Error

**If calculator/led/servo commands fail:**

- Run `scan` first to discover devices
- Make sure the specific device was found during scan
- Check peripheral serial output for error messages
- Verify I²C wiring hasn't come loose

### Permission Denied on /dev/i2c-X

```bash
# Quick fix
sudo chmod 666 /dev/i2c-1

# Permanent fix
sudo usermod -a -G i2c $USER
# Log out and back in
```

### Servos Not Moving

**Common issues:**

1. **No external power**: Servos MUST have external 5V supply
2. **Insufficient current**: Use power supply rated for at least 1A per servo
3. **Wrong PWM pins**: Servo peripheral uses D9 and D10 only
4. **Servo not attached**: Check servo peripheral serial output

### I²C Communication Errors

**If you see "Failed to read message" or similar:**

1. Check wire connections (especially GND)
2. Reduce I²C bus length if using long wires
3. Try slower bus speed (modify `crumbs_linux_init_controller` timeout parameter)
4. Ensure only one controller is accessing the bus

## When to Use Discovery vs Manual Controller

**Use Discovery Controller when:**

- ✅ Device addresses might change
- ✅ Developing/testing new peripherals
- ✅ Learning the CRUMBS system
- ✅ Need to verify device types
- ✅ Addresses are unknown

**Use Manual Controller when:**

- ✅ Addresses are fixed and known
- ✅ Performance matters (no scan delay)
- ✅ Production deployment
- ✅ Scripted/automated testing
- ✅ Address verification not needed

## Further Documentation

- **lhwit_family overview**: `../lhwit_family/README.md`
- **Calculator peripheral**: `../lhwit_family/calculator/README.md`
- **LED peripheral**: `../lhwit_family/led/README.md`
- **Servo peripheral**: `../lhwit_family/servo/README.md`
- **Manual controller**: `../manual_controller/README.md`
- **Comprehensive guide**: `../../../docs/lhwit-family.md`

## Example: Complete Session

```
$ ./lhwit_discovery_controller
═══════════════════════════════════════════════════════════
   LHWIT Discovery Controller
   Low Hardware Implementation Test - Auto-Discovery Mode
═══════════════════════════════════════════════════════════

I2C Device: /dev/i2c-1

Getting started:
  1. Run 'scan' to discover devices
  2. Use device commands (calculator, led, servo)
  3. Type 'help' for command reference

lhwit> scan
Scanning I2C bus for CRUMBS devices (0x08-0x77)...
  Found 3 device(s):
    0x10: Type 0x03 - Calculator
    0x20: Type 0x01 - LED Array
    0x30: Type 0x02 - Servo Controller

Device addresses stored. You can now use device commands.

lhwit> calculator add 42 58
OK: add(42, 58) command sent
    Use 'calculator result' to get the answer

lhwit> calculator result
Last result: 100

lhwit> led set_all 0x0A
OK: All LEDs set to 0x0A

lhwit> servo set_pos 0 90
OK: Servo 0 set to 90 degrees

lhwit> quit
Goodbye!
```
