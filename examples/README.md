# CRUMBS Examples

Examples organized by complexity. **Test I²C bus with [scanner](https://playground.arduino.cc/Main/I2cScanner/) first.**

## Tier 1: Core Usage (Beginner)

**[core_usage/](core_usage/)** — Learn CRUMBS basics without complexity

- Direct API usage (no handlers)
- Simple message sending/receiving
- Basic peripheral/controller patterns
- Self-contained sketches

**Start here** if you're new to CRUMBS.

---

## Tier 2: Handler Pattern (Intermediate)

**[handlers_usage/](handlers_usage/)** — Learn callback registration pattern

- Handler registration via `crumbs_register_handler()`
- Command routing demonstration
- SET_REPLY mechanism usage
- Mock device (no hardware complexity)

**Move here** after understanding the basics.

---

## Tier 3: Production Patterns (Advanced)

**[families_usage/](families_usage/)** — Real-world multi-module coordination

- Canonical operation headers
- Multiple device types coordinated
- Physical I²C communication
- Linux controller with discovery

**For advanced users** building complete systems.

---

## Platform Coverage

| Platform   | Tier 1         | Tier 2         | Tier 3                              |
| ---------- | -------------- | -------------- | ----------------------------------- |
| Arduino    | ✓ 5 examples   | —              | —                                   |
| PlatformIO | ✓ 2 examples   | ✓ 2 examples   | ✓ 4 peripherals (LHWIT family)      |
| Linux      | ✓ 1 controller | ✓ 1 controller | ✓ 2 controllers (discovery, manual) |

---

## Quick Start (15 min)

1. Wire: SDA↔SDA, SCL↔SCL, GND↔GND + 4.7kΩ pull-ups (**level shifter 5V↔3.3V**)
2. Upload `core_usage/arduino/hello_peripheral/` + `hello_controller/`
3. Serial (115200): `s` (send), `r` (receive)

[Troubleshooting](../README.md#troubleshooting)

---

See [docs/examples.md](../docs/examples.md) for detailed documentation.
