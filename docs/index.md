# CRUMBS Documentation

Arduino I2C communication library for controller/peripheral messaging with fixed 31-byte frames and CRC validation.

## Quick Start

```cpp
// Controller
CRUMBS controller(true);
controller.sendMessage(message, address);

// Peripheral
CRUMBS peripheral(false, 0x08);
peripheral.onReceive(handleMessage);
```

## Features

- Fixed 31-byte message format (7 float data fields)
- Controller/peripheral architecture
- Event-driven callbacks
- Built-in serialization
- CRC-8 data integrity
- Debug support

## Documentation

| File                                  | Description                    |
| ------------------------------------- | ------------------------------ |
| [Getting Started](getting-started.md) | Installation and basic usage   |
| [API Reference](api-reference.md)     | Class and method documentation |
| [Protocol](protocol.md)               | Message format specification   |
| [Examples](examples.md)               | Code examples and patterns     |

**Version**: 1.0.0 | **Author**: Cameron | **Dependencies**: Wire library, AceCRC
