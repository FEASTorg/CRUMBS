# Versioning Convention

This document describes the recommended convention for opcode `0x00` (default
reply) and version identification in CRUMBS peripherals.

## Opcode 0x00 Convention

When a peripheral is first initialized, `ctx->requested_opcode` is set to `0`.
By convention, opcode `0x00` should return **device identification and version
information**.

This enables:

- **Discovery**: Controllers can identify device types during bus scan
- **Compatibility checking**: Controllers can verify firmware versions
- **Debugging**: Easy identification of connected devices

## Recommended Payload Format

```yml
┌─────────────────┬───────────────┬───────────────┬───────────────┐
│ CRUMBS_VERSION  │  module_major │  module_minor │  module_patch │
│    (2 bytes)    │    (1 byte)   │    (1 byte)   │    (1 byte)   │
└─────────────────┴───────────────┴───────────────┴───────────────┘
```

| Field            | Size    | Description                            |
| ---------------- | ------- | -------------------------------------- |
| `CRUMBS_VERSION` | 2 bytes | Library version as u16 (little-endian) |
| `module_major`   | 1 byte  | Module firmware major version          |
| `module_minor`   | 1 byte  | Module firmware minor version          |
| `module_patch`   | 1 byte  | Module firmware patch version          |

**Total:** 5 bytes

## Peripheral Implementation

```c
#define MY_TYPE_ID      0x42
#define MODULE_VER_MAJ  1
#define MODULE_VER_MIN  0
#define MODULE_VER_PAT  0

void on_request(crumbs_context_t *ctx, crumbs_message_t *reply)
{
    switch (ctx->requested_opcode)
    {
        case 0x00:  // Default: device/version info
            crumbs_msg_init(reply, MY_TYPE_ID, 0x00);
            crumbs_msg_add_u16(reply, CRUMBS_VERSION);  // Library version
            crumbs_msg_add_u8(reply, MODULE_VER_MAJ);
            crumbs_msg_add_u8(reply, MODULE_VER_MIN);
            crumbs_msg_add_u8(reply, MODULE_VER_PAT);
            break;

        case 0x10:  // Sensor data
            // ... fill sensor data ...
            break;

        // ... other opcodes ...

        default:  // Unknown opcode
            crumbs_msg_init(reply, MY_TYPE_ID, ctx->requested_opcode);
            // Empty payload signals "unknown opcode"
            break;
    }
}
```

## Controller-Side Version Check

```c
// After discovering a peripheral at 'addr':
crumbs_message_t reply;

// Read default reply (opcode 0x00)
if (read_from_peripheral(addr, &reply) == 0 && reply.data_len >= 5)
{
    uint16_t lib_ver = reply.data[0] | (reply.data[1] << 8);
    uint8_t mod_major = reply.data[2];
    uint8_t mod_minor = reply.data[3];
    uint8_t mod_patch = reply.data[4];

    printf("Device at 0x%02X: CRUMBS v%d, module v%d.%d.%d\n",
           addr, lib_ver, mod_major, mod_minor, mod_patch);

    // Check compatibility
    if (lib_ver < 1000)  // Require CRUMBS >= 0.10.0
    {
        printf("Warning: Old CRUMBS version\n");
    }
}
```

## Helper Functions

**Core library:** CRUMBS core intentionally does not include version helpers to remain protocol-agnostic and minimal.

**Family headers:** Module families (like LHWIT) may provide convenience functions. See `lhwit_ops.h` for examples:
- `lhwit_parse_version()` - Parse 5-byte payload
- `lhwit_check_crumbs_compat()` - Verify CRUMBS version
- `lhwit_check_module_compat()` - Check major/minor compatibility

## Compatibility Rules

Follow [Semantic Versioning](https://semver.org/) for module firmware:

- **MAJOR**: Incompatible protocol changes (must match)
- **MINOR**: New commands added (peripheral >= controller required)
- **PATCH**: Bug fixes (no compatibility impact)

## See Also

- [protocol.md](protocol.md) - SET_REPLY command documentation
- [handler-guide.md](handler-guide.md) - Complete handler patterns
- [developer-guide.md](developer-guide.md) - Integration guide
