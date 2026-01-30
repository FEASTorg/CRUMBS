# Architecture Overview

## What is CRUMBS?

CRUMBS is a lightweight I²C messaging protocol enabling controllers (e.g., SBCs such as Raspberry Pi) to communicate with peripheral modules (e.g. MCUs such as Arduino) over a shared bus. The library provides the wire protocol (encoding, CRC, routing) but remains **protocol-agnostic**—it does not define specific commands. Instead, module developers create "families" of devices with their own type identifiers and command sets, allowing diverse ecosystems to be built on top of the CRUMBS transport layer.

**Key Principle**: CRUMBS is the transport layer. Module families define the application layer.

**Key Goal**: Enable modular systems where inexpensive microcontrollers act as intelligent peripherals on an I²C bus. CRUMBS handles the communication so you can focus on what each module does and how they interact, not how messages get there.

---

## Stakeholders

Four distinct roles interact with CRUMBS:

**End Users** operate CRUMBS-enabled systems without awareness of the underlying protocol. They interact with applications built on the infrastructure. _(Indirect stakeholders)_

**System Integrators** select modules, assign I²C addresses, deploy hardware, and build controller applications that orchestrate multiple peripherals. They are the primary consumers of the CRUMBS library. _(Primary stakeholders)_

**Module Developers** design and implement peripheral firmware that communicates via CRUMBS. They publish headers defining their module family's type*id and command vocabulary (opcodes). *(Application-layer developers)\_

**Library Maintainers** develop and maintain the CRUMBS core library itself, ensuring it remains portable, efficient, and protocol-agnostic. _(Platform/framework developers)_

---

## System Architecture

### Compile-Time Vocabulary Binding

Controllers and peripherals share a common "vocabulary" through header files. Module developers publish headers like:

```c
// heater_module.h (published by module developer)
#define HEATER_TYPE_ID 0x10
#define HEATER_CMD_SET_TEMP  0x01
#define HEATER_CMD_GET_TEMP  0x02
#define HEATER_CMD_VERSION   0xFD
```

System integrators compile their controller software with headers for the module families they intend to use. This establishes what commands the controller can send and how to interpret responses. The same headers are used by peripheral firmware to implement handlers.

### Physical Deployment

Modules connect to the I²C bus, each at a unique 7-bit address (0x08-0x77). Address assignment is application-defined — common methods include EEPROM configuration or firmware defaults. The controller initiates all communication.

### Discovery & Identification

**Bus Scanning**: Controller uses `crumbs_controller_scan_for_crumbs()` to find which addresses respond to CRUMBS messages. This identifies _where_ devices are, not _what_ they are.

**Type Identification**: Controller queries each discovered address (typically using reserved opcode 0xFE for IDENTIFY) to retrieve:

- `type_id` - Identifies the module family (links to known headers)
- Optional metadata (firmware version, capabilities, serial number)

**Runtime Mapping**: Controller builds a device map: `{address → type_id}`. Example:

```
0x08 → HEATER (type 0x10)
0x09 → HEATER (type 0x10)
0x0A → HEATER (type 0x10)
0x0B → FAN (type 0x20)
```

The controller now knows "three heater modules and one fan module are present."

### Operation

Controllers send messages to specific addresses using opcodes from the appropriate module family header:

```c
// Send command to heater at address 0x08
crumbs_message_t msg;
crumbs_msg_init(&msg);
msg.type_id = HEATER_TYPE_ID;
msg.opcode = HEATER_CMD_SET_TEMP;
crumbs_msg_add_float(&msg, 75.0f);  // Set target to 75°C
crumbs_controller_send(&ctx, 0x08, &msg, write_fn, io_ctx);
```

**Multiple Instances**: Multiple peripherals of the same type_id share command vocabulary but operate independently. Three heater modules might control different zones, all responding to the same opcode set.

### Optional Standard Commands

CRUMBS does **not** enforce specific opcodes. However, the library provides **opt-in helper functions** for common patterns:

- `crumbs_register_identify(ctx, opcode, ...)` - Device discovery (default 0xF8)
- `crumbs_register_ping(ctx, opcode)` - Health monitoring (default 0xFE)  
- `crumbs_register_reset(ctx, opcode, callback)` - Soft reset (default 0xFF)

**Module developer choice:** Register if you want gateway compatibility. Skip if not needed - opcodes remain available for module-specific commands. Opcode parameter is customizable if defaults conflict with your design.

**Gateway compatibility:** Gateways work best with devices using standard commands at default opcodes, but this is a guideline, not a requirement. The protocol remains fully agnostic.

### Versioning & Compatibility

Module families should implement version reporting (see [Versioning Guide](../roadmaps/archive/versioning.md)) to detect mismatches between controller headers and peripheral firmware. Controllers can query version during discovery to ensure compatibility:

- **PATCH updates** (x.y.N): Bugfixes, no command changes
- **MINOR updates** (x.N.0): New commands added (backward compatible)
- **MAJOR updates** (N.0.0): Breaking changes require controller recompilation

### Gateway Services _(Future)_

Higher-level services can expose module functionality over networks (REST, MQTT, etc.). The gateway maintains a registry of connected devices and translates network requests into CRUMBS messages, abstracting I²C details from remote clients.

Example: `/devices/heater-0x08/temperature` maps to `crumbs_controller_send(0x08, HEATER_CMD_GET_TEMP)`

---

## Design Philosophy

**Protocol-Agnostic Core**: CRUMBS does not enforce specific typeIDs or opcodes. The library handles framing, CRC, and transport — application semantics are defined by module families.

**Decentralized Standards**: There is no central registry of typeIDs. Module developers choose their own (with best-effort collision avoidance). In the future, community-driven "device classes" may emerge for common module types.

**Compile-Time Safety**: Shared headers provide type safety and documentation. If controller expects `HEATER_CMD_SET_TEMP` to take a float, the header and peripheral firmware must agree.

**Runtime Flexibility**: Controllers discover and adapt to what's physically connected. Adding a new heater module requires no code changes—just plug it in at an unused address.

**Reference Implementation**: The CRUMBS repository includes an internal "test family" of simple modules (LEDs, servos, basic sensors) with published headers. This family uses non-reserved typeIDs and serves as a working example for module developers to study. These test modules enable hardware validation and demonstrate best practices for creating module families. These also provide the library maintainers with a simple hardware test harness as well as a baseline for regression testing and performance benchmarking.
