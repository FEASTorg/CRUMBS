# CRUMBS Documentation

Arduino I2C communication library for controller/peripheral messaging with variable-length payloads and CRC validation.

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

- Variable-length payload (0–27 bytes per message)
- Controller/peripheral architecture
- Event-driven callbacks
- Per-command handler dispatch
- Message builder/reader helpers
- CRC-8 data integrity
- Debug support

## Documentation

| File                                  | Description                          |
| ------------------------------------- | ------------------------------------ |
| [Getting Started](getting-started.md) | Installation and basic usage         |
| [API Reference](api-reference.md)     | Core C API and platform HAL docs     |
| [Message Helpers](message-helpers.md) | Payload building and reading helpers |
| [Protocol](protocol.md)               | Message format specification         |
| [Examples](examples.md)               | Code examples and patterns           |
| [Developer Guide](developer-guide.md) | Architecture, integration & dev docs |
| [Linux HAL](linux.md)                 | Linux build & example notes          |

**Version**: 0.9.1 | **Author**: Cameron | **Dependencies**: Wire library (Arduino); linux-wire for Linux HAL
