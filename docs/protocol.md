# Protocol Specification

## Message Format (31 bytes)

```yml
┌──────────┬─────────────┬──────────────────┬──────────┐
│  typeID  │ commandType │     data[7]      │   crc8   │
│ (1 byte) │  (1 byte)   │   (28 bytes)     │ (1 byte) │
└──────────┴─────────────┴──────────────────┴──────────┘
```

| Field         | Size     | Description                                   |
| ------------- | -------- | --------------------------------------------- |
| `typeID`      | 1 byte   | Module type (sensor=1, motor=2, etc.)         |
| `commandType` | 1 byte   | Command (read=0, set=1, reset=2, etc.)        |
| `data[7]`     | 28 bytes | Payload data (7 floats)                       |
| `crc8`        | 1 byte   | CRC-8 over `typeID`, `commandType`, `data[7]` |

**Notes**:

- `sliceAddress` exists in the struct but is not serialized.
- CRC excludes `sliceAddress` and the CRC byte itself.

## Communication Patterns

```yml
Controller [31-byte message]> Peripheral    # Send command
Controller [I2C request]> Peripheral        # Request data
Controller <[31-byte response] Peripheral   # Response
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
- Applied to serialized payload before padding

## Timing Guidelines

- 10ms minimum between messages
- 50ms timeout for peripheral response
- 100kHz I2C clock speed (standard)
- Addresses 0x08-0x77 usable
