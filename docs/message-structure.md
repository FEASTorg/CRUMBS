# CRUMBS Message Structure Reference

**Version:** 0.10.1  
**Last Updated:** February 1, 2026

---

## Wire Format

```text
┌──────────┬──────────┬──────────┬─────────────────┬──────────┐
│  type_id │  opcode  │ data_len │   data[0..N]    │   crc8   │
│ (1 byte) │ (1 byte) │ (1 byte) │ (0–27 bytes)    │ (1 byte) │
└──────────┴──────────┴──────────┴─────────────────┴──────────┘
```

**Message Size:** 4-31 bytes  
**Maximum Payload:** 27 bytes (constrained by Arduino Wire's 32-byte buffer)

---

## Field Specifications

| Field      | Size       | Range         | Available | Notes                            |
| ---------- | ---------- | ------------- | --------- | -------------------------------- |
| `address`  | 1 byte     | `0x08`-`0x77` | 112       | I²C address (not serialized)     |
| `type_id`  | 1 byte     | `0x01`-`0xFF` | 255       | Device type identifier           |
| `opcode`   | 1 byte     | `0x01`-`0xFD` | 253       | Command identifier (per type_id) |
| `data_len` | 1 byte     | `0`-`27`      | 28        | Payload byte count               |
| `data[]`   | 0-27 bytes | N/A           | N/A       | Opaque payload                   |
| `crc8`     | 1 byte     | N/A           | N/A       | CRC-8 (polynomial 0x07)          |

---

## I²C Address Space

| Range         | Decimal | Status                           |
| ------------- | ------- | -------------------------------- |
| `0x00`-`0x07` | 0-7     | **Reserved** (I²C specification) |
| `0x08`-`0x77` | 8-119   | **Available** (112 addresses)    |
| `0x78`-`0x7F` | 120-127 | **Reserved** (I²C specification) |

Address field is **not serialized** in message (used for routing only).

---

## Type ID Space

| Range         | Decimal | Status                          |
| ------------- | ------- | ------------------------------- |
| `0x00`        | 0       | **Avoid** (uninitialized value) |
| `0x01`-`0xFF` | 1-255   | **Available** (255 type IDs)    |

**Semantics:**

- Type ID identifies device **class/type**, not individual device
- Multiple devices can share same type_id (distinguished by I²C address)
- Each type_id has independent opcode namespace

**Convention:** Allocate sequentially starting from `0x01`.

---

## Opcode Space

| Range         | Decimal | Status                              |
| ------------- | ------- | ----------------------------------- |
| `0x00`        | 0       | **Avoid** (uninitialized value)     |
| `0x01`-`0xFD` | 1-253   | **Available** (254 opcodes)         |
| `0xFE`        | 254     | **Reserved** (SET_REPLY in v0.10.0) |
| `0xFF`        | 255     | **Convention:** Error response      |

### Opcode Allocation Convention

**Purpose:** Avoid ambiguity between command opcodes and reply opcodes by allocating SET and GET operations to separate ranges.

**Key Principle:** Each type_id can choose its own allocation strategy based on device needs.

**Common Allocation Strategies:**

| Strategy           | SET Range     | GET Range     | Use Case                                      |
| ------------------ | ------------- | ------------- | --------------------------------------------- |
| **Balanced**       | `0x01`-`0x7F` | `0x80`-`0xFD` | General purpose (127 SET, 126 GET)            |
| **Sensor-heavy**   | `0x01`-`0x1F` | `0x20`-`0xFD` | Many readings, few commands (31 SET, 222 GET) |
| **Actuator-heavy** | `0x01`-`0xDF` | `0xE0`-`0xFD` | Many commands, few queries (223 SET, 30 GET)  |
| **Simple device**  | `0x01`-`0x0F` | `0x10`-`0x1F` | Minimal opcodes (15 SET, 16 GET)              |

**Example (Balanced - 0x80 split):**

```text
// Temperature controller (many readings)
SET_TEMPERATURE    = 0x01
SET_PID_PARAMS     = 0x02
GET_TEMPERATURES_0 = 0x80  // 12 temp channels → 0x80-0x87
GET_TEMPERATURES_1 = 0x81
GET_PID_PARAMS     = 0x88
```

**Example (Sensor-heavy - 0x20 split):**

```text
// Multi-channel ADC (64 channels)
SET_SAMPLE_RATE    = 0x01
GET_CHANNEL_00_07  = 0x20  // 64 channels → 0x20-0x5F
GET_CHANNEL_08_15  = 0x21
// ... up to GET_CHANNEL_56_63 = 0x5F
```

### Opcode Contexts

**1. Command Opcode** (received as `message.opcode`):

- Peripheral executes operation immediately
- Typically from SET range (low end)

**2. Target Opcode** (received in SET_REPLY `data[0]`, v0.10.1+):

- Peripheral stages reply in buffer for next I²C read
- Typically from GET range (high end)
- Returned message contains this opcode

**Multi-Opcode Pattern:** For data > 27 bytes, use sequential opcodes from GET range (e.g., `0x80`, `0x81`, `0x82` for balanced allocation).

---

## CRC-8 Specification

| Property   | Value                                           |
| ---------- | ----------------------------------------------- |
| Polynomial | `0x07` (x^8 + x^2 + x + 1)                      |
| Initial    | `0x00`                                          |
| Algorithm  | Nibble-based (4-bit chunks)                     |
| Coverage   | `type_id`, `opcode`, `data_len`, `data[]`       |
| Excluded   | `address` (not serialized), `crc8` field itself |

---

## Reserved Values (v0.10.1+)

| Field    | Value  | Purpose                              | Version |
| -------- | ------ | ------------------------------------ | ------- |
| `opcode` | `0xFE` | SET_REPLY mechanism (protocol-level) | v0.10.0 |

---

## See Also

- [protocol.md](protocol.md) - Complete protocol specification
- [message-helpers.md](message-helpers.md) - Type-safe payload helpers
