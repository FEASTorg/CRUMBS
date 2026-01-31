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

## Why No Library Helpers?

CRUMBS does not provide library functions for version checking because:

1. **Protocol-agnostic**: The payload format is a convention, not a requirement.
   Users may need different version schemes or additional fields.

2. **Minimal footprint**: Helper functions would add code size for a simple
   task that users can easily implement.

3. **Flexibility**: Some devices may not fit the standard format (e.g., devices
   with no firmware version, or with additional identification fields).

## Semantic Versioning

We recommend using [Semantic Versioning](https://semver.org/) for module firmware:

- **MAJOR**: Increment when making incompatible protocol changes
- **MINOR**: Increment when adding functionality in a backward-compatible manner
- **PATCH**: Increment when making backward-compatible bug fixes

## Extended Version Info

If 5 bytes is insufficient, you can use additional opcodes:

```c
case 0x00:  // Basic version info (5 bytes)
    // ... standard format ...
    break;

case 0x01:  // Extended device info (up to 27 bytes)
    crumbs_msg_init(reply, MY_TYPE_ID, 0x01);
    crumbs_msg_add_u16(reply, CRUMBS_VERSION);
    crumbs_msg_add_u8(reply, MODULE_VER_MAJ);
    crumbs_msg_add_u8(reply, MODULE_VER_MIN);
    crumbs_msg_add_u8(reply, MODULE_VER_PAT);
    // Additional fields:
    crumbs_msg_add_u32(reply, BUILD_TIMESTAMP);
    crumbs_msg_add_u16(reply, HARDWARE_REV);
    // ... etc ...
    break;
```

## See Also

- [protocol.md](protocol.md) - SET_REPLY command documentation
- [handler-guide.md](handler-guide.md) - Complete handler patterns
- [developer-guide.md](developer-guide.md) - Integration guide
