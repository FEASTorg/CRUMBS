# CRUMBS

CRUMBS (Communications Router and Unified Message Broker System) is an Arduino library for I2C communication between a controller and multiple peripheral devices. It provides standardized 27-byte messaging with automatic serialization for modular systems.

## Features

- **Fixed Message Format**: 27-byte messages with 6 float data fields
- **Controller/Peripheral Architecture**: One controller, multiple addressable devices
- **Event-Driven Communication**: Callback-based message handling
- **Built-in Serialization**: Automatic encoding/decoding of message structures
- **Debug Support**: Optional debug output for development and troubleshooting

## Quick Start

```cpp
#include <CRUMBS.h>

// Controller
CRUMBS controller(true);
CRUMBSMessage msg = {0, 1, 1, {1.0, 2.0, 3.0, 4.0, 5.0, 6.0}, 0};
controller.sendMessage(msg, 0x08);
```

## Installation

1. Download or clone this repository
2. Place the CRUMBS folder in your Arduino `libraries` directory
3. Include in your sketch: `#include <CRUMBS.h>`

## Hardware Requirements

- Arduino or compatible microcontroller
- I2C bus with 4.7kΩ pull-up resistors on SDA/SCL lines
- Unique addresses (0x08-0x77) for each peripheral device

## Documentation

Complete documentation is available in the [docs](docs/) directory:

- [Getting Started](docs/getting-started.md) - Installation and basic usage
- [API Reference](docs/api-reference.md) - Complete class documentation
- [Protocol Specification](docs/protocol.md) - Message format details
- [Examples](docs/examples.md) - Code examples and patterns

## Examples

The library includes complete working examples:

- **Controller Example**: Serial interface for sending commands and requesting data
- **Peripheral Example**: Message handling and response generation

## License

GPL-3.0 - see [LICENSE](LICENSE) file for details.
