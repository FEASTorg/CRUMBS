# CRUMBS

CRUMBS (Communications Router and Unified Message Broker System) is a small, portable C-based protocol for controller/peripheral I²C messaging. The project ships a C core (encoding/decoding, CRC) and thin platform HALs for Arduino and Linux so the same protocol works on microcontrollers and native hosts.

## Features

- **Fixed Message Format**: 31-byte frames with 7 float data fields
- **Controller/Peripheral Architecture**: One controller, multiple addressable devices
- **Event-Driven Communication**: Callback-based message handling
- **Built-in Serialization**: Automatic encoding/decoding of message structures
- **CRC-8 Protection**: Integrity check on every message
- **Debug Support**: Optional debug output for development and troubleshooting
- **CRUMBS-aware discovery**: Core scanner helper and HAL read adapters to discover devices that actually speak the CRUMBS protocol

## Quick Start (Arduino — C API)

```c
#include <crumbs_arduino.h>

crumbs_context_t controller_ctx;
crumbs_arduino_init_controller(&controller_ctx);

crumbs_message_t m = {};
m.type_id = 1;
m.command_type = 1;
m.data[0] = 1.0f;

crumbs_controller_send(&controller_ctx, 0x08, &m, crumbs_arduino_wire_write, NULL);
```

## Installation

1. Download or clone this repository
2. Place the CRUMBS folder in your Arduino `libraries` directory
3. No external CRC runtime dependency required — generated CRC implementations are included under `src/crc`.

The CRC code is generated with `pycrc` using the generator scripts in `scripts/` (see `scripts/generate_crc8_*`). The generator approach is inspired by AceCRC, but CRUMBS ships its own local API and does not require AceCRC at runtime.

1. Include in your sketch: `#include <crumbs_arduino.h>` (Arduino) or `#include "crumbs.h"` (C projects)

## Hardware Requirements

- Arduino or compatible microcontroller
- I2C bus with 4.7kΩ pull-up resistors on SDA/SCL lines
- Unique addresses (0x08-0x77) for each peripheral device

## Documentation

Documentation is available in the [docs](docs/) directory:

- [Getting Started](docs/getting-started.md) - Installation and basic usage
- [API Reference](docs/api-reference.md) - Core C API and platform HALs
- [Protocol Specification](docs/protocol.md) - Message format details
- [Examples](docs/examples.md) - Code examples and patterns

## Examples

Working examples for both Arduino and Linux are provided under `examples/`:

- `examples/arduino` — controller and peripheral sketches using the C API and `crumbs_arduino.h`
- `examples/linux` — native controller example using the Linux HAL

## License

GPL-3.0 - see [LICENSE](LICENSE) file for details.
