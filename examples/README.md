# CRUMBS Examples

Examples are organized into three tiers based on complexity:

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

| Platform   | Tier 1         | Tier 2         | Tier 3                  |
| ---------- | -------------- | -------------- | ----------------------- |
| Arduino    | ✓ 5 examples   | —              | (Roadmap-03)            |
| PlatformIO | ✓ 2 examples   | ✓ 2 examples   | (Roadmap-03)            |
| Linux      | ✓ 1 controller | ✓ 1 controller | (Roadmap-03)            |

---

## Quick Start

### Arduino Users

1. Install CRUMBS via Library Manager
2. Open `examples/core_usage/arduino/simple_peripheral/`
3. Upload and open Serial Monitor

### PlatformIO Users

```bash
cd examples/core_usage/platformio/simple_peripheral
pio run --target upload
```

### Linux Users

```bash
cd examples/core_usage/linux/simple_controller
make
./simple_controller /dev/i2c-1
```

---

See [docs/examples.md](../docs/examples.md) for detailed documentation.
