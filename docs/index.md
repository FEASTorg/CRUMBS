# CRUMBS Documentation

Arduino I2C communication library for controller/peripheral messaging with fixed 31-byte frames and CRC validation.

## Quick Start

```c
// Controller (Arduino HAL)
#include <crumbs_arduino.h>
crumbs_context_t ctx;
crumbs_arduino_init_controller(&ctx);

// Peripheral (Arduino HAL)
// attach callbacks and initialise the peripheral
crumbs_context_t pctx;
crumbs_arduino_init_peripheral(&pctx, 0x08);
```

## Features

- Fixed 31-byte message format (7 float data fields)
- Controller/peripheral architecture
- Event-driven callbacks
- Built-in serialization
- CRC-8 data integrity
- Debug support

## Documentation

| File                                  | Description                      |
| ------------------------------------- | -------------------------------- |
| [Getting Started](getting-started.md) | Installation and basic usage     |
| [API Reference](api-reference.md)     | Core C API and platform HAL docs |
| [Protocol](protocol.md)               | Message format specification     |
| [Examples](examples.md)               | Code examples and patterns       |
| [Linux HAL](linux.md)                 | Linux build & example notes      |

**Version**: 1.0.0 | **Author**: Cameron | **Dependencies**: Wire library (Arduino) — CRC code is included in-tree (no AceCRC runtime required)
