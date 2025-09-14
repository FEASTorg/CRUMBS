# CRUMBS Documentation

Complete documentation for the CRUMBS Arduino I2C communication library.

## What is CRUMBS?

CRUMBS provides standardized I2C communication between a controller and multiple peripheral devices using:

- **Fixed message format**: 27-byte messages with 6 float data values
- **Controller/Peripheral architecture**: One controller, multiple addressable slices
- **Event-driven communication**: Callback-based message handling
- **Built-in serialization**: Automatic encoding/decoding of message structures
- **Debug support**: Optional debug output for troubleshooting

## Quick Start

```cpp
// Controller
CRUMBS controller(true);
controller.sendMessage(message, address);

// Peripheral
CRUMBS peripheral(false, 0x08);
peripheral.onReceive(handleMessage);
```

## Documentation Files

| File                                         | Description                              |
| -------------------------------------------- | ---------------------------------------- |
| **[🚀 Getting Started](getting-started.md)** | Installation and basic usage tutorial    |
| **[📖 API Reference](api-reference.md)**     | Complete class and method documentation  |
| **[🔌 Protocol](protocol.md)**               | Message format and communication details |
| **[💡 Examples](examples.md)**               | Example projects and usage patterns      |

## Library Info

- **Version**: 1.0.0 | **Author**: Cameron | **Platform**: Arduino (all architectures)
- **Dependencies**: Wire library (included with Arduino)
- **Repository**: [GitHub](https://github.com/FEASTorg/CRUMBS)
