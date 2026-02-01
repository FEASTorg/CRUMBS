# CHANGELOG

All notable changes to CRUMBS are documented in this file.

---

## [0.10.0] - SET_REPLY Mechanism

### Added

- **SET_REPLY Command (0xFE)**: Library-handled opcode for reply staging
  - `CRUMBS_CMD_SET_REPLY` constant defined in `crumbs.h`
  - Library auto-intercepts 0xFE and stores target opcode
  - NOT dispatched to user handlers or `on_message` callbacks
  - Wire format: `[type_id][0xFE][0x01][target_opcode][CRC8]`

- **Requested Opcode Field** (`ctx->requested_opcode`)
  - New `uint8_t requested_opcode` field in `crumbs_context_s` (+1 byte)
  - Initialized to 0 in `crumbs_init()` (by convention: device info)
  - User's `on_request` callback switches on this value

- **Scan with Types** (`crumbs_controller_scan_for_crumbs_with_types()`)
  - Same as `crumbs_controller_scan_for_crumbs()` but returns `type_id` array
  - Enables device type discovery in a single scan
  - Optional `types` parameter (may be NULL)

- **New Tests**
  - `tests/test_set_reply.c`: Unit tests for SET_REPLY mechanism (8 tests)
  - `tests/test_reply_flow.c`: Integration tests for query flow (6 tests)
  - Added `scan_with_types` tests to `test_scan_fake.c`

- **Documentation**
  - `docs/versioning.md`: New document for opcode 0x00 convention
  - `docs/protocol.md`: Added Reserved Opcodes section
  - `docs/developer-guide.md`: Added SET_REPLY Pattern section
  - `docs/handler-guide.md`: Updated `on_request` example with case 0x00
  - `docs/examples.md`: Reorganized to reflect three-tier learning path

### Examples

- **Three-Tier Reorganization**: Examples restructured into progressive learning path
  - `examples/core_usage/`: Basic protocol learning (Tier 1)
  - `examples/handlers_usage/`: Production callback patterns (Tier 2)
  - `examples/families_usage/`: Real device families (Tier 3, planned)

- **Mock Handler Examples** (Type ID 0x10): Demonstrating handler registration and SET_REPLY
  - PlatformIO: `mock_peripheral/`, `mock_controller/` (Nano + ESP32)
  - Linux: `mock_controller/`
  - Shared `mock_ops.h` header with SET operations (ECHO, SET_HEARTBEAT, TOGGLE) and GET operations (GET_ECHO, GET_STATUS, GET_INFO)

- **Moved Examples**: Simple examples relocated to `core_usage/`
  - Arduino: `simple_peripheral/`, `simple_controller/`, `display_peripheral/`, `display_controller/`
  - PlatformIO: `simple_peripheral/`, `simple_controller/`
  - Linux: `simple_controller/`

### Memory Impact

- +1 byte per `crumbs_context_t` (for `requested_opcode` field)

---

## [0.9.5] - Foundation Revision

### Added

- **Version Header** (`src/crumbs_version.h`): Runtime version query and conditional compilation
  - `CRUMBS_VERSION_MAJOR`, `CRUMBS_VERSION_MINOR`, `CRUMBS_VERSION_PATCH`
  - `CRUMBS_VERSION_STRING` - String representation ("0.9.5")
  - `CRUMBS_VERSION` - Integer version (905 = major*10000 + minor*100 + patch)

- **Platform Timing API**: Foundation for multi-frame timeout detection (v0.11.0)
  - `crumbs_platform_millis_fn` typedef in `crumbs_i2c.h`
  - `crumbs_arduino_millis()` - Arduino millisecond timer
  - `crumbs_linux_millis()` - Linux CLOCK_MONOTONIC millisecond timer
  - `CRUMBS_ELAPSED_MS(start, now)` - Wraparound-safe elapsed time calculation
  - `CRUMBS_TIMEOUT_EXPIRED(start, now, timeout_ms)` - Wraparound-safe timeout check

- **Platform Timing Documentation**: See `docs/developer-guide.md` for usage examples

### Changed

- **BREAKING**: Renamed `crumbs_msg.h` → `crumbs_message_helpers.h` for clarity
  - Eliminates confusion with `crumbs_message.h` (structure definition)
  - All includes and documentation updated

- **BREAKING**: Renamed `command_type` field → `opcode` in `crumbs_message_t`
  - Semantically correct terminology for protocol operations
  - Updated all references across codebase, examples, tests, and documentation

---

## [0.9.4] - Message Builder/Reader Helpers

### Added

- **Message Builder/Reader API** (`src/crumbs_message_helpers.h`): Zero-overhead inline helpers for structured payload construction and reading
  - `crumbs_msg_init(msg, type_id, opcode)`: Initialize a message with header fields
  - `crumbs_msg_add_u8/u16/u32()`: Append unsigned integers (little-endian)
  - `crumbs_msg_add_i8/i16/i32()`: Append signed integers (little-endian)
  - `crumbs_msg_add_float()`: Append 32-bit float (native byte order)
  - `crumbs_msg_add_bytes()`: Append raw byte arrays
  - `crumbs_msg_read_u8/u16/u32()`: Read unsigned integers from payload
  - `crumbs_msg_read_i8/i16/i32()`: Read signed integers from payload
  - `crumbs_msg_read_float()`: Read 32-bit float from payload
  - `crumbs_msg_read_bytes()`: Read raw byte arrays from payload
  - All functions return 0 on success, -1 on bounds overflow
  - Header-only design: `static inline` functions for zero call overhead

- **Contract Header Pattern**: Demonstrated in `examples/handlers_usage/mock_ops.h`
  - Shared header defining TYPE_ID, opcodes, and helper functions
  - Controller and peripheral both include the same contract
  - Demonstrates the "copy and customize" pattern for device families

- **Configurable Handler Table Size** (`CRUMBS_MAX_HANDLERS`):
  - Compile-time option to control handler dispatch table size
  - Default: 16 (good balance of RAM vs handler capacity)
  - Set lower values (e.g., 4 or 8) to reduce RAM on constrained devices
  - Set to 0 to disable handler dispatch entirely
  - Uses O(n) linear search (fast for typical handler counts)

### Documentation

- New documentation: `docs/message-helpers.md` — comprehensive guide to crumbs_message_helpers.h
- Updated `docs/api-reference.md` with message helper API
- Updated `docs/index.md` to reflect variable-length payload (was outdated)

### Notes

- This layer composes with the existing handler dispatch system (0.8.x)
- No breaking changes — all existing code continues to work
- Float encoding uses native byte order; document this if crossing architectures

## [0.8.x] - Command Handler Dispatch System v0

### Added

- **Command Handler Dispatch System**: Per-command-type handler registration for structured message processing
  - `crumbs_handler_fn` typedef: `void (*)(crumbs_context_t*, const crumbs_message_t* message, void* user_data)`
  - `crumbs_register_handler(ctx, opcode, fn, user_data)`: Register handler for a command type
  - `crumbs_unregister_handler(ctx, opcode)`: Clear handler for a command type
  - Handlers are invoked after `on_message` callback (if both are set)
  - O(n) dispatch via configurable handler table (default 16 entries)
- **Handler Examples**: See `examples/handlers_usage/` for mock device examples

## [0.7.x] - Variable-Length Payload

### Breaking Changes

This release introduces a **breaking change** to the CRUMBS wire format and C API.
All code using CRUMBS must be updated.

#### Wire Format

| Old Format (31 bytes fixed) | New Format (4–31 bytes variable) |
| --------------------------- | -------------------------------- |
| type_id (1)                 | type_id (1)                      |
| opcode (1)                  | opcode (1)                       |
| data (28) — 7 × float32     | data_len (1) — 0–27              |
| crc8 (1)                    | data (0–27) — raw bytes          |
|                             | crc8 (1)                         |

#### API Changes

| Old Constant/Field         | New Constant/Field                      |
| -------------------------- | --------------------------------------- |
| `CRUMBS_DATA_LENGTH` (7)   | `CRUMBS_MAX_PAYLOAD` (27)               |
| `CRUMBS_MESSAGE_SIZE` (31) | `CRUMBS_MESSAGE_MAX_SIZE` (31)          |
| `float data[7]`            | `uint8_t data_len` + `uint8_t data[27]` |

#### Migration Guide

1. **Update message construction:**

   ```c
   // Old
   msg.data[0] = 1.0f;
   msg.data[1] = 2.0f;

   // New — raw bytes; encode floats manually
   float val = 1.0f;
   msg.data_len = sizeof(float);
   memcpy(msg.data, &val, sizeof(float));
   ```

2. **Update message reading:**

   ```c
   // Old
   float val = msg.data[0];

   // New — decode from bytes
   float val;
   if (msg.data_len >= sizeof(float)) {
       memcpy(&val, msg.data, sizeof(float));
   }
   ```

3. **Update buffer size constants:**

   ```c
   // Old
   uint8_t buf[CRUMBS_MESSAGE_SIZE];

   // New
   uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE];
   ```

4. **Note encoder return value:**
   - `crumbs_encode_message()` now returns the actual encoded length (4 + data_len), not a fixed size.

### Added

- Variable-length payload support (0–27 bytes per message)
- `data_len` field in `crumbs_message_t` for explicit payload size
- Raw byte payloads — application defines interpretation

### Changed

- Wire format now includes explicit `data_len` field
- CRC computed over header + actual payload bytes only
- Encoder returns variable length based on payload size
- All examples and documentation updated for byte API

### Removed

- Fixed float-based payload (`float data[7]`)
- `CRUMBS_DATA_LENGTH` and `CRUMBS_MESSAGE_SIZE` constants

---

## [0.1.x – 0.6.x] - Early Development (2024)

### Summary

Initial development of the CRUMBS protocol and library:

- **0.1.x**: Initial protocol design with fixed 31-byte frames and 7-float payload
- **0.2.x**: Basic encode/decode implementation with CRC-8 integrity checking
- **0.3.x**: Arduino HAL with Wire library integration (controller/peripheral)
- **0.4.x**: Event-driven callbacks (`on_message`, `on_request`)
- **0.5.x**: Linux HAL with linux-wire dependency for Raspberry Pi support
- **0.6.x**: I²C bus scanner helpers, documentation improvements

These versions established the core architecture used in all later releases.
