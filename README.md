# CRUMBS

CRUMBS (Communications Router and Unified Message Broker System) is an Arduino library for I2C communication between a controller and multiple peripheral devices. It provides standardized 31-byte messaging with automatic serialization and CRC validation for modular systems.

## Features

- **Fixed Message Format**: 31-byte frames with 7 float data fields
- **Controller/Peripheral Architecture**: One controller, multiple addressable devices
- **Event-Driven Communication**: Callback-based message handling
- **Built-in Serialization**: Automatic encoding/decoding of message structures
- **CRC-8 Protection**: Integrity check on every message
- **Debug Support**: Optional debug output for development and troubleshooting

## Quick Start

```cpp
#include <CRUMBS.h>

// Controller
CRUMBS controller(true);
CRUMBSMessage msg = {0, 1, 1, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f}, 0};
controller.sendMessage(msg, 0x08);
```

## Installation

1. Download or clone this repository
2. Place the CRUMBS folder in your Arduino `libraries` directory
3. No external CRC dependency required — the repository includes generated CRC implementations under `src/crc`.

The CRC generation step uses [pycrc](https://github.com/tpircher/pycrc) and the generator scripts in `scripts/generate_crc8_arduino.py`.

The generator and its approach are inspired by [AceCRC](https://github.com/bxparks/AceCRC), but CRUMBS includes a simplified local API so the library does not require that external dependency and additional namespace layers.
4. Include in your sketch: `#include <CRUMBS.h>`

## Hardware Requirements

- Arduino or compatible microcontroller
- I2C bus with 4.7kΩ pull-up resistors on SDA/SCL lines
- Unique addresses (0x08-0x77) for each peripheral device

## Documentation

Documentation is available in the [docs](docs/) directory:

- [Getting Started](docs/getting-started.md) - Installation and basic usage
- [API Reference](docs/api-reference.md) - Class documentation
- [Protocol Specification](docs/protocol.md) - Message format details
- [Examples](docs/examples.md) - Code examples and patterns

## Examples

The library includes working examples to get you started:

- **Controller Example**: Serial interface for sending commands and requesting data
- **Peripheral Example**: Message handling and response generation

## License

GPL-3.0 - see [LICENSE](LICENSE) file for details.
