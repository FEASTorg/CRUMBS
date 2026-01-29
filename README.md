# CRUMBS

[![CI](https://github.com/FEASTorg/CRUMBS/actions/workflows/ci.yml/badge.svg)](https://github.com/FEASTorg/CRUMBS/actions/workflows/ci.yml)
[![Pages](https://github.com/FEASTorg/FEASTorg.github.io/actions/workflows/pages.yml/badge.svg)](https://feastorg.github.io/crumbs/)
[![Doc-check](https://github.com/FEASTorg/CRUMBS/actions/workflows/doccheck.yml/badge.svg)](https://github.com/FEASTorg/CRUMBS/actions/workflows/doccheck.yml)

[![License: GPL-3.0-or-later](https://img.shields.io/badge/License-GPL--3.0--or--later-blue.svg)](./LICENSE)
[![Language](https://img.shields.io/badge/language-C%20%7C%20C%2B%2B-00599C)](./)
[![CMake](https://img.shields.io/badge/build-CMake-064F8C)](./)

![Linux](https://img.shields.io/badge/Linux-FCC624?logo=linux&logoColor=black)
![PlatformIO](https://img.shields.io/badge/PlatformIO-E37B0D?logo=platformio&logoColor=white&style=flat)
![Arduino](https://img.shields.io/badge/​Arduino-00979D?logo=arduino&logoColor=white)

CRUMBS (Communications Router and Unified Message Broker System) is a small, portable C-based protocol for controller/peripheral I²C messaging. The project ships a C core (encoding/decoding, CRC) and thin platform HALs for Arduino and Linux so the same protocol works on microcontrollers and native hosts.

## Features

- **Variable-Length Message Format**: 4–31 byte frames with opaque byte payloads (0–27 bytes)
- **Controller/Peripheral Architecture**: One controller, multiple addressable devices
- **Per-Command Handler Dispatch**: Register handlers for specific command types
- **Message Builder/Reader Helpers**: Type-safe payload construction via `crumbs_message_helpers.h`
- **Event-Driven Communication**: Callback-based message handling
- **CRC-8 Protection**: Integrity check on every message
- **CRUMBS-aware Discovery**: Core scanner helper to find devices that speak CRUMBS

## Quick Start (Arduino — C API)

```c
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

crumbs_context_t controller_ctx;
crumbs_arduino_init_controller(&controller_ctx);

crumbs_message_t m;
crumbs_msg_init(&m);
m.type_id = 1;
m.opcode = 1;

// Type-safe payload building
crumbs_msg_add_float(&m, 25.5f);  // e.g. change a temperature value
crumbs_msg_add_u8(&m, 0x01);      // e.g. configure the channel index

crumbs_controller_send(&controller_ctx, 0x08, &m, crumbs_arduino_wire_write, NULL);
```

## Installation

1. Download or clone this repository
2. Place the CRUMBS folder in your Arduino `libraries` directory
3. Include in your sketch: `#include <crumbs_arduino.h>` (Arduino) or `#include "crumbs.h"` (C projects)

No external dependencies required — CRC implementations are included under `src/crc`.

## Hardware Requirements

- Arduino or compatible microcontroller
- I2C bus with 4.7kΩ pull-up resistors on SDA/SCL lines
- Unique addresses (0x08-0x77) for each peripheral device

## Documentation

Documentation is available in the [docs](docs/) directory:

- [Getting Started](docs/getting-started.md) - Installation and basic usage
- [API Reference](docs/api-reference.md) - Core C API and platform HALs
- [Handler Guide](docs/handler-guide.md) - Per-command handler dispatch tutorial
- [Message Helpers](docs/message-helpers.md) - Payload building and reading helpers
- [Protocol Specification](docs/protocol.md) - Message format details
- [Examples](docs/examples.md) - Code examples and patterns

## Examples

Working examples for Arduino, Linux, and PlatformIO are provided under `examples/`:

- `examples/arduino/` — Arduino sketches (simple, display, and handler-based examples)
- `examples/linux/` — Native Linux controller examples using the Linux HAL
- `examples/platformio/` — PlatformIO projects (ready to build with `pio run`)
- `examples/common/` — Shared command header definitions (LED, servo)

## License

GPL-3.0 - see [LICENSE](LICENSE) file for details.
