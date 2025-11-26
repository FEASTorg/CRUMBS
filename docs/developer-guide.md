# CRUMBS — Developer Guide

This guide explains the design, architecture, and implementation details of the CRUMBS protocol and codebase. It is intended for engineers integrating CRUMBS into microcontrollers (MCUs) and single-board computers (SBCs), or for contributors who want to understand, extend, and maintain the project.

Short background (why CRUMBS)

- Cam built CRUMBS to solve a practical problem for the BREAD system: a set of function cards (each with an MCU) are connected to a supervising SBC (Raspberry Pi) over I²C via a backplane and wiring that can be noisy. The project needed a compact, robust, easy-to-implement protocol so SBCs and MCUs can reliably exchange structured sensor/command data across I²C with CRC protection and small, portable platform HALs.

Goals and constraints

- Small and portable C core (easy to include on microcontrollers)
- Deterministic fixed-size wire format for simple framing and predictable behavior
- CRC-8 for integrity across noisy wires
- Thin HALs for Arduino (Wire) and Linux (linux-wire), with guarded implementations so a single repo can be used for both targets
- Practical scan/discovery primitives that can find devices that _actually_ speak CRUMBS
- Low memory usage and no dynamic allocations in the core (suitable for small MCUs)

High-level architecture

- Core protocol (C): encoding/decoding, CRC implementation, message helpers, context and callback model — located under `src/core` and public header `src/crumbs.h`
- HALs: platform-specific adapters that map the core to the platform I²C primitives
  - Arduino HAL: `src/hal/arduino/` and public header `src/crumbs_arduino.h`
  - Linux HAL: `src/hal/linux/` and public header `src/crumbs_linux.h` (uses linux-wire; guarded for non-Linux builds)
- Examples: `examples/arduino` and `examples/linux` show controller and peripheral usage
- Documentation: `docs/` contains the API reference, getting-started notes, and examples documentation

Message format (wire format)

- Fixed serialized length: 31 bytes (CRUMBS_MESSAGE_SIZE)
  - type_id: 1 byte
  - command_type: 1 byte
  - data: 7 float32 values (7 × 4 = 28 bytes)
  - crc8: 1 byte (CRC-8 computed over type_id + command_type + data)

Design rationale

- Fixed-size frames simplify read/parse loops on both controllers and peripherals — frames either read in full or fail fast.
- CRC-8 is a good tradeoff on short bus lengths where noise may occur and a single byte per frame keeps overhead small.
- Callback-driven peripheral handling keeps peripheral code simple and event-driven for MCU environments.

Role definitions

- Controller: initiates sends and requests; uses `crumbs_controller_send()` for transmissions. It runs on the supervising device (SBC or MCU acting as master).
- Peripheral (Slice): responds to requests and receives messages. Uses callbacks installed via `crumbs_set_callbacks()` and HAL-specific receive handlers.

Core API highlights

- `crumbs_init(ctx, role, address)` — initialize role.
- `crumbs_controller_send(ctx, addr, &msg, write_fn, ctx)` — encode and send a CRUMBS frame using a platform write primitive.
- `crumbs_peripheral_handle_receive()` — decode inbound bytes at the peripheral and invoke on_message callback when decode succeeds.
- `crumbs_peripheral_build_reply()` — build a reply to send in on_request.
- CRC helpers and stats: `crumbs_decode_message()` returns 0 on success; `crumbs_get_crc_error_count()` and `crumbs_last_crc_ok()` provide diagnostics.

HAL primitives and integration

The core is platform-agnostic and relies on the HAL to provide I²C primitives:

- Write primitive: `crumbs_i2c_write_fn` — start + addr(write) + data + stop. Used by controllers to send frames.
- Read primitive: `crumbs_i2c_read_fn` — attempt to read up to a given number of bytes from a target address. Added so the core scanner can drive platform reads in a portable way.

Arduino HAL (Wire)

- Public headers: `src/crumbs_arduino.h`
- Implementations: `src/hal/arduino/crumbs_i2c_arduino.cpp`
- Notable functions
  - `crumbs_arduino_init_controller()` / `crumbs_arduino_init_peripheral()` — setup controller/peripheral roles on Wire
  - `crumbs_arduino_wire_write()` — write adapter for crumbs_controller_send
  - `crumbs_arduino_read()` — read helper used by the CRUMBS-aware scanner (Wire.requestFrom() + timeout loop)
  - `crumbs_arduino_scan()` — generic address-level scanner (address ACK or data-phase probe)

Linux HAL (linux-wire)

- Public headers: `src/crumbs_linux.h`
- Implementations: `src/hal/linux/crumbs_i2c_linux.c`
- Notes: the Linux HAL depends on linux-wire. The Linux files are guarded so the repository can be compiled by Arduino toolchains without pulling in linux-wire.
- Notable functions
  - `crumbs_linux_init_controller()` / `crumbs_linux_close()` — open/close the linux-wire bus and set timeout hints
  - `crumbs_linux_i2c_write()` — linux-wire write adapter
  - `crumbs_linux_read()` — linux-wire read wrapper used by core scanner
  - `crumbs_linux_scan()` — generic address-level scanner for linux-wire

Discovery and scanning

CRUMBS provides two scanning layers

1. HAL generic scanner (`crumbs_arduino_scan`, `crumbs_linux_scan`) — detects addresses by probing the address phase or small write/read operations. This finds any device that ACKs an address, not necessarily CRUMBS devices.

2. Core CRUMBS-aware scanner (`crumbs_controller_scan_for_crumbs`) — uses a read primitive to attempt to read a full CRUMBS message and runs `crumbs_decode_message()` to verify CRC and frame format. Optionally, in non-strict mode the scanner will send a probe write before the read to stimulate replies from devices that only respond after being written to.

Strict vs non-strict probing

- strict=1: prefer a read-only data-phase probe (good to check devices that respond to reads)
- strict=0: do address/zero-length probes and then optionally send a small write to encourage a reply

How CRUMBS identifies a device as CRUMBS

- The core scanner accepts an address only when a read returns a valid CRUMBS frame (correct size and CRC and decodable into a crumbs_message_t). This avoids false positives caused by devices that merely ACK the bus.

Recommended production hardening

- Add an explicit PING/PONG handshake (command_type set aside for discovery) for higher confidence and to report device metadata/version. This reduces the already small risk of accidentally accepting non-CRUMBS devices.
- Validate meaningful fields (e.g., type_id, command ranges) after decode to help avoid spurious matches

Examples & patterns

- Sending a frame (controller):

```c
crumbs_context_t ctx;
crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);
crumbs_message_t m = { .type_id = 1, .command_type = 1 };
crumbs_controller_send(&ctx, 0x08, &m, crumbs_arduino_wire_write, &Wire);
```

- Peripheral setup (Arduino):

```c
crumbs_context_t ctx;
crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x08);
crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
crumbs_arduino_init_peripheral(&ctx, 0x08);
```

- CRUMBS-aware scan (controller):

```c
uint8_t found[32];
int n = crumbs_controller_scan_for_crumbs(&ctx, 0x03, 0x77, 0, crumbs_arduino_wire_write, crumbs_arduino_read, &Wire, found, sizeof(found), 50000);
```

Testing and CI

- Build matrix should include both Linux builds (with linux-wire) and Arduino builds (ensure Arduino-friendly files compile without linux-wire included).

- Unit / integration options
  - Static unit tests for encode/decode/CRC in CI (fast, no hardware)
  - Integration tests using Linux I²C loopback hardware or a hardware lab (requires attached devices)

Integration tips and pitfalls

- Bus reliability: use pull-ups, twisted pairs, good wiring, and proper grounding — CRC reduces but doesn't eliminate the need for good wiring
- Device address selection: avoid conflicts and use the scanner to detect collisions early
- Real-time constraints: CRUMBS frames are fixed-size — plan reads/writes with delays and hardware constraints in mind

Maintenance notes for contributors

- Keep the core strictly C and free of platform headers. HALs may use C++ where platform bindings (Arduino) are convenient.
- Add tests near `src/core` for encode/decode/CRC changes; instruction generators for CRC are in `scripts/`.
- Use guarded implementations in HALs for platform-specific includes (example: linux-wire guarded under **linux** so Arduino builds stay clean).

Where to start reading code for contributions

- Start in `src/core/crumbs_core.c` for the core logic and message handling
- Read `src/crumbs.h` and `src/crumbs_i2c.h` for public APIs and HAL interfaces
- Inspect `src/hal/arduino/crumbs_i2c_arduino.cpp` and `src/hal/linux/crumbs_i2c_linux.c` for platform glue
- Examples under `examples/` show concrete usage and are the fastest way to iterate with real hardware

Appendix: quick reference (API / filenames)

- Core public header: `src/crumbs.h`
- I²C primitives: `src/crumbs_i2c.h` (write_fn + read_fn + scan_fn)
- Arduino HAL: `src/crumbs_arduino.h`, `src/hal/arduino/crumbs_i2c_arduino.cpp`
- Linux HAL: `src/crumbs_linux.h`, `src/hal/linux/crumbs_i2c_linux.c`
- Examples: `examples/arduino`, `examples/linux`

If anything here looks wrong, incomplete, or too verbose, tell me where you'd like it tightened — I can remove duplication, expand the handshake pattern details, or add an appendix with message diagrams and example packet bytes for use with protocol analyzers.
