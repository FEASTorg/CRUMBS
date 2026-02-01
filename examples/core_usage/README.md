# Core Usage Examples (Tier 1)

**Audience:** First-time users, Arduino beginners  
**Purpose:** Learn CRUMBS basics without complexity

These examples demonstrate direct API usage without handlers or complex patterns. Each example is self-contained and can be understood in isolation.

---

## Arduino Examples

| Example                                                              | Description                                        |
| -------------------------------------------------------------------- | -------------------------------------------------- |
| [simple_peripheral/](arduino/simple_peripheral/)                     | Basic peripheral responding to controller commands |
| [simple_controller/](arduino/simple_controller/)                     | Basic controller sending commands via serial input |
| [simple_peripheral_noncrumbs/](arduino/simple_peripheral_noncrumbs/) | Raw I²C peripheral (no CRUMBS) for comparison      |
| [display_peripheral/](arduino/display_peripheral/)                   | Multi-byte data peripheral (display values)        |
| [display_controller/](arduino/display_controller/)                   | Multi-byte data controller                         |

### Getting Started (Arduino)

1. Install the CRUMBS library via Arduino Library Manager or copy `src/` to your libraries folder
2. Open any example sketch in Arduino IDE
3. Select your board and port
4. Upload and open Serial Monitor (115200 baud)

---

## PlatformIO Examples

| Example                                             | Description                     |
| --------------------------------------------------- | ------------------------------- |
| [simple_peripheral/](platformio/simple_peripheral/) | Basic peripheral for PlatformIO |
| [simple_controller/](platformio/simple_controller/) | Basic controller for PlatformIO |

### Getting Started (PlatformIO)

```bash
# Build and upload peripheral
cd examples/core_usage/platformio/simple_peripheral
pio run --target upload

# Build and upload controller
cd examples/core_usage/platformio/simple_controller
pio run --target upload
```

---

## Linux Examples

| Example                                        | Description                               |
| ---------------------------------------------- | ----------------------------------------- |
| [simple_controller/](linux/simple_controller/) | Linux controller using linux-wire library |

### Getting Started (Linux)

```bash
cd examples/core_usage/linux/simple_controller
make
./simple_controller /dev/i2c-1
```

**Note:** Requires [linux-wire](https://github.com/AnotherDaniel/linux-wire) library installed.

---

## Learning Path

1. **Start here:** `simple_peripheral` + `simple_controller` pair
2. **Understand the difference:** Compare `simple_peripheral` with `simple_peripheral_noncrumbs`
3. **Multi-byte data:** `display_peripheral` + `display_controller` pair
4. **Next step:** Move to [handlers_usage/](../handlers_usage/) (Tier 2) for callback patterns
