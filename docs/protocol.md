# Protocol Specification

## Message Format (27 bytes)

```yml
┌──────────┬─────────────┬───────────────┬─────────────┐
│  typeID  │ commandType │   data[6]     │ errorFlags  │
│ (1 byte) │  (1 byte)   │  (24 bytes)   │  (1 byte)   │
└──────────┴─────────────┴───────────────┴─────────────┘
```

| Field         | Size     | Description                            |
| ------------- | -------- | -------------------------------------- |
| `typeID`      | 1 byte   | Module type (sensor=1, motor=2, etc.)  |
| `commandType` | 1 byte   | Command (read=0, set=1, reset=2, etc.) |
| `data[6]`     | 24 bytes | Payload data (6 floats)                |
| `errorFlags`  | 1 byte   | Error/status flags                     |

**Note**: `sliceAddress` exists in struct but is not serialized.

## Communication Patterns

```yml
Controller [27-byte message]> Peripheral    # Send command
Controller [I2C request]> Peripheral        # Request data
Controller <[27-byte response] Peripheral   # Response
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

### Error Flags (bit flags)

- `0x01`: General error
- `0x02`: Communication error
- `0x04`: Hardware fault
- `0x08`: Invalid command
- `0x10`: Out of range data
- `0x20`: Calibration needed

## Timing Guidelines

- 10ms minimum between messages
- 50ms timeout for peripheral response
- 100kHz I2C clock speed (standard)
- Addresses 0x08-0x77 usable
