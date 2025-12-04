# CHANGELOG

## [0.8.0] - Command Handler Dispatch System

### Added

- **Command Handler Dispatch System**: Per-command-type handler registration for structured message processing
  - `crumbs_handler_fn` typedef: `void (*)(crumbs_context_t*, uint8_t command_type, const uint8_t* data, uint8_t data_len, void* user_data)`
  - `crumbs_register_handler(ctx, command_type, fn, user_data)`: Register handler for a command type
  - `crumbs_unregister_handler(ctx, command_type)`: Clear handler for a command type
  - Handlers are invoked after `on_message` callback (if both are set)
  - O(1) dispatch via 256-entry lookup table per context
- **Handler Examples**:
  - `examples/arduino/handler_peripheral_example/` - Arduino peripheral using per-command handlers (LED control, echo)
  - `examples/linux/handler_controller/` - Linux controller CLI for handler peripheral

## [0.7.x] - Variable-Length Payload

### Breaking Changes

This release introduces a **breaking change** to the CRUMBS wire format and C API.
All code using CRUMBS must be updated.

#### Wire Format

| Old Format (31 bytes fixed) | New Format (4–31 bytes variable) |
| --------------------------- | -------------------------------- |
| type_id (1)                 | type_id (1)                      |
| command_type (1)            | command_type (1)                 |
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
