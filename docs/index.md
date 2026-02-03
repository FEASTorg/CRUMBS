# CRUMBS Documentation

Arduino I²C communication library for controller/peripheral messaging with variable-length payloads and CRC validation.

## Quick Start

```c
// Controller (Arduino HAL)
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

crumbs_context_t ctx;
crumbs_arduino_init_controller(&ctx);

crumbs_message_t msg;
crumbs_msg_init(&msg, 0x01, 0x01);  // type_id=1, opcode=1
crumbs_msg_add_u8(&msg, 1);         // payload: LED ON

crumbs_controller_send(&ctx, 0x08, &msg, crumbs_arduino_wire_write, NULL);
```

```c
// Peripheral (Arduino HAL)
#include <crumbs_arduino.h>

crumbs_context_t ctx;

void on_message(crumbs_context_t *ctx, const crumbs_message_t *msg) {
    // Process incoming command
    if (msg->data_len >= 1) {
        digitalWrite(LED_BUILTIN, msg->data[0] ? HIGH : LOW);
    }
}

void on_request(crumbs_context_t *ctx, crumbs_message_t *reply) {
    // Build reply for I²C read
    reply->type_id = 1;
    reply->opcode = 0;
    reply->data_len = 1;
    reply->data[0] = digitalRead(LED_BUILTIN);
}

void setup() {
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
}
```

## Features

- **Variable-length payload** (0–27 bytes per message)
- **Controller/peripheral architecture** (one controller, multiple devices)
- **Per-command handler dispatch** (register handlers for specific opcodes)
- **Message builder/reader helpers** (type-safe payload construction)
- **Event-driven callbacks** (message and request handling)
- **CRC-8 data integrity** (automatic validation)
- **CRUMBS-aware discovery** (find devices that speak the protocol)
- **Platform support** (Arduino, PlatformIO, Linux)

---

## Core Documentation

### Getting Started

| Document                              | Description                                                       |
| ------------------------------------- | ----------------------------------------------------------------- |
| [Platform Setup](platform-setup.md)   | Installation and configuration for Arduino, PlatformIO, and Linux |
| [Protocol Specification](protocol.md) | Wire format, versioning, CRC-8, reserved opcodes                  |
| [Examples](examples.md)               | Three-tier learning path with platform coverage                   |

### Reference

| Document                          | Description                                                      |
| --------------------------------- | ---------------------------------------------------------------- |
| [API Reference](api-reference.md) | Complete C API, handler dispatch, message helpers, platform HALs |
| [Architecture](architecture.md)   | Design philosophy, stakeholder roles, system architecture        |
| [LHWIT Family](lhwit-family.md)   | Reference implementation (LEDs, servos, calculator, display)     |

### Developer

| Document                                          | Description                                  |
| ------------------------------------------------- | -------------------------------------------- |
| [CONTRIBUTING.md](../CONTRIBUTING.md)             | How to build, test, and contribute to CRUMBS |
| [Character Usage Guide](character-usage-guide.md) | ASCII vs Unicode in documentation            |
| [Doxygen Style Guide](doxygen-style-guide.md)     | In-source documentation standards            |

### Meta

| Document                        | Description                            |
| ------------------------------- | -------------------------------------- |
| [PlatformIO](platformio.md)     | Registry publishing and CI integration |
| [CHANGELOG.md](../CHANGELOG.md) | Version history and release notes      |

---

## Architecture Overview

**CRUMBS is a transport layer protocol.** It handles framing, CRC, and routing but does not define specific commands. Instead, developers create **module families**—collections of device types with shared headers defining type identifiers and command vocabularies.

**Key Constraint:** A single I²C bus uses one module family. The controller is compiled with that family's headers and only understands those type_ids and opcodes.

**Stakeholders:**

- **End Users** — Operate systems built on CRUMBS (indirect)
- **System Integrators** — Build controller applications (primary)
- **Module Developers** — Implement peripheral firmware (application layer)
- **Library Maintainers** — Develop the core library (platform layer)

See [Architecture](architecture.md) for complete design documentation.

---

## Quick Reference

### Message Structure

```c
typedef struct {
    uint8_t address;      // Device I²C address (not serialized)
    uint8_t type_id;      // Device/module type identifier
    uint8_t opcode;       // Command/query opcode
    uint8_t data_len;     // Payload length (0-27)
    uint8_t data[27];     // Payload buffer
    uint8_t crc8;         // CRC-8 checksum
} crumbs_message_t;
```

**Wire format:** `[type_id:1][opcode:1][data_len:1][data:0-27][crc8:1]` (4-31 bytes)

### Core Functions

```c
// Initialization
void crumbs_init(crumbs_context_t *ctx, crumbs_role_t role, uint8_t address);
void crumbs_set_callbacks(crumbs_context_t *ctx,
                          crumbs_message_cb_t on_message,
                          crumbs_request_cb_t on_request,
                          void *user_data);

// Controller operations
int crumbs_controller_send(const crumbs_context_t *ctx,
                           uint8_t target_addr,
                           const crumbs_message_t *msg,
                           crumbs_i2c_write_fn write_fn,
                           void *write_ctx);

// Peripheral operations
int crumbs_peripheral_handle_receive(crumbs_context_t *ctx,
                                     const uint8_t *buffer,
                                     size_t len);
int crumbs_peripheral_build_reply(crumbs_context_t *ctx,
                                  uint8_t *out_buf,
                                  size_t out_buf_len,
                                  size_t *out_len);

// Handler dispatch
int crumbs_register_handler(crumbs_context_t *ctx,
                            uint8_t opcode,
                            crumbs_handler_fn fn,
                            void *user_data);
```

### Message Helpers

```c
#include <crumbs_message_helpers.h>

// Building messages
crumbs_msg_init(&msg, type_id, opcode);
crumbs_msg_add_u8(&msg, value);
crumbs_msg_add_u16(&msg, value);
crumbs_msg_add_u32(&msg, value);
crumbs_msg_add_float(&msg, value);

// Reading payloads
crumbs_msg_read_u8(data, len, offset, &out);
crumbs_msg_read_u16(data, len, offset, &out);
crumbs_msg_read_u32(data, len, offset, &out);
crumbs_msg_read_float(data, len, offset, &out);
```

---

## Archived Documentation

Historical and technical deep-dive documents are in [archive/](archive/):

- `developer-notes.md` — Historical design decisions
- `crc.md` — CRC-8 implementation details

---

**Version**: 0.10.3  
**Author**: Cameron K. Brooks  
**Dependencies**: Wire library (Arduino), linux-wire (Linux HAL)  
**License**: GPL-3.0-or-later
