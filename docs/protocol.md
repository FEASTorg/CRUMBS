# Protocol Specification

## Message Format

### Structure (27 bytes serialized)

```yml
┌──────────┬─────────────┬───────────────┬─────────────┐
│  typeID  │ commandType │   data[6]     │ errorFlags  │
│ (1 byte) │  (1 byte)   │  (24 bytes)   │  (1 byte)   │
└──────────┴─────────────┴───────────────┴─────────────┘
```

### Fields

| Field         | Size     | Type     | Description                            |
| ------------- | -------- | -------- | -------------------------------------- |
| `typeID`      | 1 byte   | uint8_t  | Module type (sensor=1, motor=2, etc.)  |
| `commandType` | 1 byte   | uint8_t  | Command (read=0, set=1, reset=2, etc.) |
| `data[6]`     | 24 bytes | float[6] | Payload data array                     |
| `errorFlags`  | 1 byte   | uint8_t  | Error/status flags                     |

**Note**: `sliceAddress` field exists in the struct but is NOT serialized - it's used for routing only.

## Communication Patterns

### Controller → Peripheral (Send)

```yml
Controller ──[27-byte message]──> Peripheral
```

### Controller ← Peripheral (Request)

```yml
Controller ──[I2C request]──> Peripheral
Controller <──[27-byte response]── Peripheral
```

## Standard Values

### Module Types

| Type     | Value | Description            |
| -------- | ----- | ---------------------- |
| Generic  | 0     | Unspecified            |
| Sensor   | 1     | Environmental sensors  |
| Actuator | 2     | Motors, servos, relays |
| Display  | 3     | LCD, LED, OLED         |
| Input    | 4     | Buttons, encoders      |

### Command Types

| Command        | Value | Description            |
| -------------- | ----- | ---------------------- |
| Read Data      | 0     | Request/provide status |
| Set Parameters | 1     | Configure settings     |
| Reset          | 2     | Reset device state     |
| Calibrate      | 3     | Trigger calibration    |

### Error Flags (Bit Flags)

| Bit | Value | Description         |
| --- | ----- | ------------------- |
| 0   | 0x01  | General error       |
| 1   | 0x02  | Communication error |
| 2   | 0x04  | Hardware fault      |
| 3   | 0x08  | Invalid command     |
| 4   | 0x10  | Out of range data   |
| 5   | 0x20  | Calibration needed  |

## Data Field Usage Examples

### Sensor (Type 1)

- `data[0]`: Primary reading (temperature, etc.)
- `data[1]`: Secondary reading (humidity, etc.)
- `data[2]`: Timestamp or sequence number
- `data[3-5]`: Additional readings

### Motor (Type 2)

- `data[0]`: Speed/velocity
- `data[1]`: Position/angle
- `data[2]`: Acceleration
- `data[3]`: Current/power
- `data[4-5]`: Limits/setpoints

## Timing Guidelines

- **Between messages**: 10ms minimum
- **Request timeout**: 50ms for peripheral response
- **Clock speed**: 100kHz (standard), 400kHz (fast mode)

## Implementation Notes

- Float values use platform's native representation (typically IEEE 754)
- All devices must use same endianness
- Only one CRUMBS instance per Arduino (singleton pattern)
- I2C addresses 0x08-0x77 usable (avoid 0x00-0x07, 0x78-0x7F)
