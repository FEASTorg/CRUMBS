# Protocol Specification

## Message Format (Variable Length: 4–31 bytes)

```yml
┌──────────┬─────────────┬──────────┬─────────────────┬──────────┐
│  typeID  │ commandType │ data_len │   data[0..N]    │   crc8   │
│ (1 byte) │  (1 byte)   │ (1 byte) │ (0–27 bytes)    │ (1 byte) │
└──────────┴─────────────┴──────────┴─────────────────┴──────────┘
```

| Field         | Size       | Description                                              |
| ------------- | ---------- | -------------------------------------------------------- |
| `typeID`      | 1 byte     | Module type (sensor=1, motor=2, etc.)                    |
| `commandType` | 1 byte     | Command (read=0, set=1, reset=2, etc.)                   |
| `data_len`    | 1 byte     | Number of payload bytes (0–27)                           |
| `data[]`      | 0–27 bytes | Opaque payload data (user-defined format)                |
| `crc8`        | 1 byte     | CRC-8 over `typeID`, `commandType`, `data_len`, `data[]` |

**Notes**:

- `sliceAddress` exists in the struct but is not serialized.
- Maximum frame size is 31 bytes to fit within Arduino Wire's 32-byte buffer.
- CRC excludes `sliceAddress` and the CRC byte itself.
- Payload is opaque bytes; applications can encode floats, ints, structs, etc.

## Communication Patterns

```yml
Controller [4–31 byte message]> Peripheral    # Send command
Controller [I2C request]> Peripheral          # Request data
Controller <[4–31 byte response] Peripheral   # Response
```

## Standard Values

### Module Types

- `0`: Generic
- `1`: Sensor
- `2`: Actuator
- `3`: Display
- `4`: Input

### Command Types

- `0`: Read data
- `1`: Set parameters
- `2`: Reset
- `3`: Calibrate

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
m.command_type = 1;

float value = 3.14f;
m.data_len = sizeof(float);
memcpy(m.data, &value, sizeof(float));
```
