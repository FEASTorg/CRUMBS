# Protocol Specification

## Message Format (Variable Length: 4–31 bytes)

```yml
┌──────────┬──────────┬──────────┬─────────────────┬──────────┐
│  type_id │  opcode  │ data_len │   data[0..N]    │   crc8   │
│ (1 byte) │ (1 byte) │ (1 byte) │ (0–27 bytes)    │ (1 byte) │
└──────────┴──────────┴──────────┴─────────────────┴──────────┘
```

| Field      | Size       | Description                                          |
| ---------- | ---------- | ---------------------------------------------------- |
| `type_id`  | 1 byte     | Module type identifier (user-defined)                |
| `opcode`   | 1 byte     | Command/operation identifier (user-defined)          |
| `data_len` | 1 byte     | Number of payload bytes (0–27)                       |
| `data[]`   | 0–27 bytes | Opaque payload data (user-defined format)            |
| `crc8`     | 1 byte     | CRC-8 over `type_id`, `opcode`, `data_len`, `data[]` |

**Notes**:

- `address` exists in the struct but is not serialized (used for routing at the application layer).
- Maximum frame size is 31 bytes to fit within Arduino Wire's 32-byte buffer.
- CRC excludes `address` and the CRC byte itself.
- Payload is opaque bytes; applications can encode floats, ints, structs, etc.
- **Opcode convention**: Separate SET and GET ranges to avoid reply ambiguity. Common: SET `0x01`-`0x7F`, GET `0x80`-`0xFD`. Each type_id can choose allocation based on needs.

## Communication Patterns

```yml
Controller [4–31 byte message]> Peripheral    # Send command
Controller [I2C request]> Peripheral          # Request data
Controller <[4–31 byte response] Peripheral   # Response
```

### CRC

- Polynomial: 0x07 (CRC-8)
- Calculated using `crc8_nibble_calculate()` (crc8_nibble variant)
- Applied to serialized header + payload (bytes 0 through 2 + data_len)

## Timing Guidelines

- 10ms minimum between messages
- 50ms timeout for peripheral response
- 100kHz I2C clock speed (standard)
- Addresses 0x08-0x77 usable

## Example: Encoding a Float

To send a float value in the payload:

```c
crumbs_message_t m = {0};
m.type_id = 1;
m.opcode = 1;

float value = 3.14f;
m.data_len = sizeof(float);
memcpy(m.data, &value, sizeof(float));
```
