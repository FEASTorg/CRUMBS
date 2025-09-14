# CRUMBS Documentation

Arduino I2C communication library for controller/peripheral messaging with fixed 27-byte messages.

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

- Fixed 27-byte message format (6 float data fields)
- Controller/peripheral architecture
- Event-driven callbacks
- Built-in serialization
- Debug support

## Documentation

| File                                  | Description                    |
| ------------------------------------- | ------------------------------ |
| [Getting Started](getting-started.md) | Installation and basic usage   |
| [API Reference](api-reference.md)     | Class and method documentation |
| [Protocol](protocol.md)               | Message format specification   |
| [Examples](examples.md)               | Code examples and patterns     |

**Version**: 1.0.0 | **Author**: Cameron | **Dependencies**: Wire library
