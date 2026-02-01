# Usage Families (Tier 3)

**Audience:** Advanced users building complete systems  
**Purpose:** Real-world multi-module coordination with actual hardware

## Overview

Usage families demonstrate complete, multi-device I²C systems with both peripheral and controller implementations. Unlike Tier 1 (handler patterns) and Tier 2 (single-device examples), usage families show how multiple devices coordinate to form a functional system.

**What you'll find here:**

- **Canonical protocol definitions** - Shared header files defining operations for device families
- **Multiple peripheral implementations** - 3+ related devices working together
- **Controller applications** - Interactive tools to command the full system
- **Complete hardware guides** - Wiring, power, and safety considerations
- **Real testing procedures** - How to validate multi-device systems

## Tier Comparison

| Tier                         | Focus                               | Complexity | Hardware |
| ---------------------------- | ----------------------------------- | ---------- | -------- |
| **Tier 1** (handlers_usage)  | Handler patterns, single operations | Low        | Optional |
| **Tier 2** (device_examples) | Single complete devices             | Medium     | Required |
| **Tier 3** (families_usage)  | Multi-device systems                | High       | Required |

## Available Families

### LHWIT Family (Low Hardware Implementation Test)

Three-device reference system demonstrating different handler patterns:

- **Calculator** (Type 0x03) - Function-style interface (ADD/SUB/MUL/DIV + history)
- **LED Array** (Type 0x01) - State-query interface (4 LEDs + blink control)
- **Servo Controller** (Type 0x02) - Position-control interface (2 servos + sweep)

**Controllers:**

- **discovery_controller** - Auto-discovers devices by type, interactive commands
- **manual_controller** - Uses hardcoded addresses from config, faster startup

**Documentation:** [lhwit_family/README.md](lhwit_family/README.md) | [Comprehensive Guide](../../docs/lhwit-family.md)

## When to Use Usage Families

**Use Tier 3 examples when:**

- Building multi-device I²C systems
- Learning device coordination patterns
- Understanding different handler philosophies (function/state/position)
- Need reference implementations for custom families
- Testing full system integration

**Start with Tier 1-2 if:**

- New to CRUMBS (start with handlers_usage)
- Learning basic I²C concepts
- Building single-device applications
- Just exploring the library

## Structure

Each family includes:

```
family_name/
├── device_name_ops.h        # Canonical operation definitions (shared)
├── family_ops.h             # Convenience header (includes all ops)
├── device1/                 # PlatformIO peripheral implementation
├── device2/
├── device3/
├── README.md                # Family overview
discovery_controller/        # Auto-discovery interactive tool
manual_controller/           # Config-based interactive tool
```

## Next Steps

1. **Explore lhwit_family** - Read [lhwit_family/README.md](lhwit_family/README.md) for hardware requirements
2. **Build peripherals** - Flash calculator, LED, servo to Arduino Nano boards
3. **Test controllers** - Use discovery or manual controller to interact with devices
4. **Read comprehensive guide** - See [docs/lhwit-family.md](../../docs/lhwit-family.md) for full details

## Design Philosophy

**Canonical Headers:** Operation definitions live in shared headers (e.g., `calculator_ops.h`). Both peripherals and controllers include the same header, ensuring protocol consistency.

**Multiple Patterns:** Different devices demonstrate different handler approaches - function-style (calculator), state-query (LED), position-control (servo). Choose the pattern that fits your use case.

**Interactive Testing:** Controllers provide command-line interfaces for manual testing. This is more flexible than automated sequences and better for debugging hardware issues.

**Two Controller Approaches:** Discovery controller scans for devices (flexible), manual controller uses config (fast). Both show valid patterns for production systems.
