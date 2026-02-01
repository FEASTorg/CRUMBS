# LHWIT Manual Controller

Hardcoded-address interactive controller for the LHWIT (Low Hardware Implementation Test) device family. This controller uses pre-configured addresses from `config.h` instead of performing bus scans, providing faster startup and simpler operation when device addresses are known.

## Overview

The manual controller showcases the **hardcoded configuration pattern** where device addresses are loaded from a configuration file at startup. This is ideal for:

- **Production deployments**: Known, fixed addresses for reliable operation
- **Automated testing**: No scan delay, immediate command availability
- **Performance**: Fastest startup time (no bus scan overhead)
- **Simplicity**: No runtime address discovery logic

**Contrast with discovery_controller**: The discovery controller scans the I²C bus at runtime to find devices, which is more flexible but requires an explicit scan step. Choose manual mode when addresses are stable and known.

## Supported Devices

This controller works with the lhwit_family peripherals:

| Device Type          | Type ID | Default Address | Configuration Constant |
| -------------------- | ------- | --------------- | ---------------------- |
| **Calculator**       | 0x03    | 0x10            | `CALCULATOR_ADDR`      |
| **LED Array**        | 0x01    | 0x20            | `LED_ADDR`             |
| **Servo Controller** | 0x02    | 0x30            | `SERVO_ADDR`           |

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

## Configuration

Before building, edit [config.h](config.h) to match your hardware setup.

### Setting Device Addresses

Open `config.h` and modify these constants:

```c
/** Calculator peripheral I²C address */
#define CALCULATOR_ADDR 0x10

/** LED peripheral I²C address */
#define LED_ADDR 0x20

/** Servo peripheral I²C address */
#define SERVO_ADDR 0x30
```

### Finding Your Peripheral Addresses

**Method 1: Check peripheral source code**

Each peripheral defines its address in `main.cpp`:

- [calculator/src/main.cpp](../lhwit_family/calculator/src/main.cpp) - Look for `#define PERIPHERAL_ADDR`
- [led/src/main.cpp](../lhwit_family/led/src/main.cpp) - Look for `#define PERIPHERAL_ADDR`
- [servo/src/main.cpp](../lhwit_family/servo/src/main.cpp) - Look for `#define PERIPHERAL_ADDR`

**Method 2: Use i2cdetect tool**

```bash
# Scan I²C bus 1 for all connected devices
i2cdetect -y 1

# Output shows addresses in hex grid
#      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
# 00:          -- -- -- -- -- -- -- -- -- -- -- -- --
# 10: 10 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 20: -- 20 -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 30: 30 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

**Method 3: Run discovery_controller**

The discovery controller can scan and identify devices:

```bash
cd ../discovery_controller/build
./lhwit_discovery_controller
lhwit> scan
```

Copy the discovered addresses to `config.h`.

### Setting I²C Bus Path

If your I²C bus is not `/dev/i2c-1`, edit this line in `config.h`:

```c
/** Default I²C bus device path */
#define DEFAULT_I2C_BUS_PATH "/dev/i2c-1"  // Change to /dev/i2c-0 etc.
```

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

# The executable will be: ./lhwit_manual_controller
```

### Troubleshooting Build Issues

**Error: "crumbs.h not found"**

- The CMakeLists.txt assumes CRUMBS_PATH is 3 levels up
- Set manually: `cmake .. -DCRUMBS_PATH=/path/to/CRUMBS`

**Error: "linux-wire not found"**

- CRUMBS must be built with `CRUMBS_ENABLE_LINUX_HAL=ON`
- Rebuild CRUMBS with Linux support enabled

**Error: "lhwit_ops.h not found"**

- Canonical headers must exist in parent directory: `../lhwit_family/`
- Verify Day 1-2 peripheral implementations are complete

## Usage

### Basic Workflow

1. **Configure addresses**: Edit `config.h` to match your hardware
2. **Rebuild**: Run `make` in build directory if config changed
3. **Flash peripherals**: Upload firmware to all three Arduino Nanos
4. **Connect hardware**: Wire the I²C bus as shown above
5. **Run controller**: Execute the manual controller binary
6. **Issue commands**: Control devices immediately (no scan required)

### Running the Controller

```bash
# Use default I²C device (from config.h, typically /dev/i2c-1)
./lhwit_manual_controller

# Or specify custom I²C device
./lhwit_manual_controller /dev/i2c-0
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
help                    Show all available commands with configured addresses
quit, exit              Exit the controller
```

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

lhwit> servo get_pos
Servo positions: Servo 0 = 90°, Servo 1 = 45°

lhwit> servo sweep 0 1 0 180 10
OK: Servo 0 sweeping from 0 to 180 degrees (step 10)
```

## Troubleshooting

### Command Returns "Failed to write/read message"

**Check configuration:**

1. Verify addresses in `config.h` match peripheral definitions
2. Run `i2cdetect -y 1` to confirm devices are at expected addresses
3. Check peripheral serial output for startup messages

**Common mismatches:**

- `config.h` has 0x10 but peripheral is at 0x20
- Wrong I²C bus (using /dev/i2c-0 when devices are on /dev/i2c-1)
- Peripheral not running or crashed

**Fix:**

- Update `config.h` with correct addresses
- Rebuild: `cd build && make`
- Re-run controller

### How to Verify Peripheral Addresses

**Option 1: Serial Monitor** (during peripheral boot):

```
--- Calculator Peripheral (Type 0x03, Addr 0x10) ---
I2C peripheral initialized successfully
Listening on address 0x10
```

**Option 2: i2cdetect**:

```bash
i2cdetect -y 1
# Look for hex addresses in the grid output
```

**Option 3: Discovery Controller**:

```bash
cd ../discovery_controller/build
./lhwit_discovery_controller
lhwit> scan
```

### No Response from Peripherals

**Check:**

1. All peripherals are powered on and running
2. I²C wiring is correct (SDA, SCL, GND)
3. No loose connections on breadboard
4. Peripherals have been flashed with correct firmware
5. I²C bus permissions are correct

### Servos Not Moving

**Common issues:**

1. **No external power**: Servos MUST have external 5V supply
2. **Insufficient current**: Use power supply rated for at least 1A per servo
3. **Wrong configuration**: Verify `SERVO_ADDR` matches servo peripheral's actual address

### Wrong Device Responds

**Symptom**: Calculator command affects LED, or other unexpected behavior

**Cause**: Address mismatch in `config.h`

**Fix**:

1. Check each peripheral's address definition in their `main.cpp`
2. Update `config.h` to match
3. Rebuild: `cd build && make`

### Permission Denied on /dev/i2c-X

```bash
# Quick fix
sudo chmod 666 /dev/i2c-1

# Permanent fix
sudo usermod -a -G i2c $USER
# Log out and back in
```

## Advantages vs Discovery Controller

### Manual Controller Advantages ✅

- **Faster startup**: No bus scan delay
- **Immediate availability**: All commands work immediately after launch
- **Simpler logic**: No "device not found" checks needed
- **Production-ready**: Fixed addresses for reliable operation
- **Clearer errors**: Address mismatches fail quickly and obviously

### Discovery Controller Advantages ✅

- **Flexible addresses**: Works regardless of peripheral addresses
- **Type verification**: Confirms device types during scan
- **Development-friendly**: No recompilation when addresses change
- **Unknown addresses**: Useful when exploring new hardware

## When to Use Manual Controller

**Choose manual controller when:**

- ✅ Device addresses are fixed and documented
- ✅ Production deployment with stable configuration
- ✅ Automated testing scripts (no interactive scan)
- ✅ Performance matters (minimize startup delay)
- ✅ Simpler codebase preferred

**Choose discovery controller when:**

- ✅ Addresses might change between sessions
- ✅ Developing/testing new peripherals
- ✅ Learning the CRUMBS system
- ✅ Need type verification for safety
- ✅ Addresses are unknown or variable

## Configuration Change Workflow

When peripheral addresses change:

1. **Update config.h**:

   ```c
   #define CALCULATOR_ADDR 0x11  // Changed from 0x10
   ```

2. **Rebuild**:

   ```bash
   cd build
   make
   ```

3. **Test**:

   ```bash
   ./lhwit_manual_controller
   lhwit> calculator add 5 10
   lhwit> calculator result
   ```

4. **Verify** (if issues):
   ```bash
   i2cdetect -y 1  # Confirm new address
   ```

## Further Documentation

- **lhwit_family overview**: `../lhwit_family/README.md`
- **Calculator peripheral**: `../lhwit_family/calculator/README.md`
- **LED peripheral**: `../lhwit_family/led/README.md`
- **Servo peripheral**: `../lhwit_family/servo/README.md`
- **Discovery controller**: `../discovery_controller/README.md`
- **Comprehensive guide**: `../../../docs/lhwit-family.md`

## Example: Complete Session

```
$ ./lhwit_manual_controller
═══════════════════════════════════════════════════════════
   LHWIT Manual Controller
   Low Hardware Implementation Test - Configured Address Mode
═══════════════════════════════════════════════════════════

I2C Device: /dev/i2c-1

Configured Addresses (from config.h):
  Calculator: 0x10 (Type 0x03)
  LED Array:  0x20 (Type 0x01)
  Servo:      0x30 (Type 0x02)

Ready! Type 'help' for command reference.

lhwit> calculator add 42 58
OK: add(42, 58) command sent
    Use 'calculator result' to get the answer

lhwit> calculator result
Last result: 100

lhwit> led set_all 0x0A
OK: All LEDs set to 0x0A

lhwit> led get_state
LED state: 0x0A (1010)
  LED 0: OFF, LED 1: ON, LED 2: OFF, LED 3: ON

lhwit> servo set_pos 0 90
OK: Servo 0 set to 90 degrees

lhwit> servo get_pos
Servo positions: Servo 0 = 90°, Servo 1 = 0°

lhwit> quit
Goodbye!
```

## Configuration Reference

Default configuration in `config.h`:

```c
#define CALCULATOR_ADDR 0x10        // Calculator peripheral
#define LED_ADDR 0x20               // LED array peripheral
#define SERVO_ADDR 0x30             // Servo controller peripheral
#define DEFAULT_I2C_BUS_PATH "/dev/i2c-1"  // Raspberry Pi default
```

Customize these values before building to match your hardware setup.
