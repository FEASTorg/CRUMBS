# Architecture Overview

## What is CRUMBS?

CRUMBS is a lightweight I²C messaging protocol enabling controllers (e.g., SBCs such as Raspberry Pi) to communicate with peripheral modules (e.g. MCUs such as Arduino) over a shared bus. The library provides the wire protocol (encoding, CRC, routing) but remains **protocol-agnostic**—it does not define specific commands. Instead, developers create "module families"—collections of device types with shared headers defining type identifiers and command vocabularies. A controller is compiled for one family and can communicate with any module type within that family.

**Key Principle**: CRUMBS is the transport layer. Module families define the application layer.

**Key Constraint**: A single I²C bus uses one module family. The controller is compiled with that family's headers and only understands those type_ids and opcodes.

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

A module family defines a set of device types, each with a unique `type_id` and command vocabulary. Controllers and peripherals share this vocabulary through header files:

```c
// Example: lhwit_family headers (reference implementation)
// led_module.h
#define LED_TYPE_ID          0x01
#define LED_CMD_SET_ALL      0x01
#define LED_CMD_SET_ONE      0x02
#define LED_OP_GET_STATE     0x80

// servo_module.h
#define SERVO_TYPE_ID        0x02
#define SERVO_CMD_SET_POS    0x01
#define SERVO_OP_GET_POS     0x80

// calculator_module.h
#define CALC_TYPE_ID         0x03
#define CALC_CMD_ADD         0x01
#define CALC_OP_GET_RESULT   0x80
```

The controller is compiled with headers for its target family. This establishes what module types exist (`type_id` values) and what commands each type understands (opcodes). The same headers are used by peripheral firmware to implement handlers.

### Physical Deployment

Modules connect to the I²C bus, each at a unique 7-bit address (0x08-0x77). Address assignment is application-defined — common methods include EEPROM configuration or firmware defaults. The controller initiates all communication.

### Discovery & Identification

**Bus Scanning**: Controller uses `crumbs_controller_scan_for_crumbs()` to find which addresses respond with valid CRUMBS messages. The response contains the peripheral's `type_id`, which tells the controller what type of module is at that address.

**How It Works**:

1. Scan attempts to read from each address
2. If response is a valid CRUMBS message (correct CRC), device is discovered
3. The decoded message contains `type_id` - the controller looks this up in its compiled headers to know what type of module it is (LED, servo, calculator, etc.)
4. No special IDENTIFY command needed - any valid response reveals the type

**Enhanced Discovery** (v0.10.0+): Use `crumbs_controller_scan_for_crumbs_with_types()` to get both addresses and type_ids in one call:

```c
uint8_t addrs[16];
uint8_t types[16];
int count = crumbs_controller_scan_for_crumbs_with_types(
    &ctx, 0x08, 0x77, 0, write_fn, read_fn, io_ctx,
    addrs, types, 16, 10000);

for (int i = 0; i < count; i++) {
    printf("Address 0x%02X -> type_id 0x%02X\n", addrs[i], types[i]);
}
```

**Runtime Mapping**: Controller builds a device map: `{address → type_id}`. Example:

```
0x20 → LED (type 0x01)
0x21 → LED (type 0x01)
0x30 → SERVO (type 0x02)
0x40 → CALCULATOR (type 0x03)
```

The controller now knows "two LED modules, one servo, and one calculator are present" and can send the appropriate commands to each based on its type.

### Operation

Controllers send messages to specific addresses using opcodes from the module's header:

```c
// Send command to servo at address 0x30
crumbs_message_t msg;
crumbs_msg_init(&msg);
msg.type_id = SERVO_TYPE_ID;
msg.opcode = SERVO_CMD_SET_POS;
crumbs_msg_add_uint8(&msg, 90);  // Set position to 90°
crumbs_controller_send(&ctx, 0x30, &msg, write_fn, io_ctx);
```

**Multiple Instances**: Multiple peripherals of the same type_id share command vocabulary but operate independently. Two LED modules at different addresses respond to the same opcodes but control different physical LEDs.

### Optional Standard Commands

CRUMBS does **not** enforce specific opcodes. However, the library reserves one opcode:

- `0xFE` - **SET_REPLY**: Controller tells peripheral which data to stage for the next I²C read

Additionally, module developers may use common conventions:

- `0xFF` - Error responses (recommended convention)
- `0x80-0xFD` - GET opcodes (reply data types)
- `0x00-0x7F` - SET/command opcodes

### Versioning & Compatibility

Module families should implement version reporting (see [Versioning Guide](../roadmaps/archive/versioning.md)) to detect mismatches between controller headers and peripheral firmware. Controllers can query version during discovery to ensure compatibility:

- **PATCH updates** (x.y.N): Bugfixes, no command changes
- **MINOR updates** (x.N.0): New commands added (backward compatible)
- **MAJOR updates** (N.0.0): Breaking changes require controller recompilation

### Gateway Services _(Future)_

Higher-level services can expose module functionality over networks (REST, MQTT, etc.). The gateway maintains a registry of connected devices and translates network requests into CRUMBS messages, abstracting I²C details from remote clients.

Example: `/devices/servo-0x30/position` maps to `crumbs_controller_send(0x30, SERVO_CMD_SET_POS)`

---

## Design Philosophy

**Protocol-Agnostic Core**: CRUMBS does not enforce specific type_ids or opcodes. The library handles framing, CRC, and transport — application semantics are defined by module families.

**One Family Per Bus**: A controller is compiled for one module family and only understands that family's type_ids and opcodes. You cannot mix modules from different families on the same I²C bus.

**Compile-Time Safety**: Shared headers provide type safety and documentation. If controller expects `SERVO_CMD_SET_POS` to take a uint8, the header and peripheral firmware must agree.

**Runtime Flexibility**: Controllers discover and adapt to what's physically connected. Adding a new LED module requires no code changes—just plug it in at an unused address.

**Reference Implementation**: The CRUMBS repository includes an internal "lhwit_family" of simple modules (LEDs, servos, calculators) with published headers. This family serves as a working example for developers to study, enables hardware validation, and provides a baseline for regression testing.
