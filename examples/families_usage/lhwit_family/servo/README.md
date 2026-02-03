# Servo Controller Peripheral

3-channel servo controller peripheral demonstrating position-control interface pattern.

## Hardware

- **Board:** Arduino Nano (ATmega328P)
- **I2C Address:** 0x30
- **I2C Pins:** A4 (SDA), A5 (SCL)
- **Type ID:** 0x02
- **Servo Pins:**
  - Servo 0: Pin 9 (D9)
  - Servo 1: Pin 10 (D10)

## ⚠️ IMPORTANT: External Power Required

**Servos draw significant current (up to 1A per servo under load).** The Arduino Nano's onboard regulator cannot safely power multiple servos. **Always use an external 5V power supply** connected to servo power lines (red wire).

### Power Wiring

```text
External 5V PSU (+) → Servo power rails (red wires)
External 5V PSU (-) → Arduino GND + Servo ground (brown/black wires)
Arduino D9/D10      → Servo signal (orange/white wires)
```

## Features

- **Position Control:** Set servo angles (0–180 degrees)
- **Speed Limiting:** Smooth movement at configurable speeds (0=instant, 1–20=slow)
- **Sweep Patterns:** Automatic back-and-forth movement for demos/testing
- **2 Channels:** Control 2 servos independently

## Building

### PlatformIO

```bash
cd servo
pio run                    # Build
pio run -t upload          # Upload to Arduino Nano
pio device monitor         # View serial output
```

## Operations

### SET Commands (Control Servos)

| Opcode | Command   | Payload                                    | Description                               |
| ------ | --------- | ------------------------------------------ | ----------------------------------------- |
| 0x01   | SET_POS   | `[servo_idx:u8][position:u8]`              | Set target position (0–180°)              |
| 0x02   | SET_SPEED | `[servo_idx:u8][speed:u8]`                 | Set movement speed (0=instant, 1–20=slow) |
| 0x03   | SWEEP     | `[idx:u8][en:u8][min:u8][max:u8][step:u8]` | Configure sweep pattern                   |

### GET Commands (Query State via SET_REPLY)

| Opcode | Command   | Reply                    | Description                 |
| ------ | --------- | ------------------------ | --------------------------- |
| 0x80   | GET_POS   | `[pos0:u8][pos1:u8]`     | Current positions (degrees) |
| 0x81   | GET_SPEED | `[speed0:u8][speed1:u8]` | Speed limits                |

## Example Usage

From a Linux controller:

```c
#include "servo_ops.h"

/* Set servo 0 to 90 degrees (instant) */
servo_send_set_pos(&ctx, 0x30, write_fn, io, 0, 90);

/* Set servo 1 speed to 5 (slow movement) */
servo_send_set_speed(&ctx, 0x30, write_fn, io, 1, 5);

/* Move servo 1 to 45 degrees (smooth movement at speed=5) */
servo_send_set_pos(&ctx, 0x30, write_fn, io, 1, 45);

/* Configure servo 2 to sweep 0–180° */
servo_send_sweep(&ctx, 0x30, write_fn, io, 2, 1, 0, 180, 2);

/* Query current positions */
servo_query_get_pos(&ctx, 0x30, write_fn, io);
uint8_t positions[3];
/* ...read response and parse positions... */
```

## Sweep Pattern

When sweep is enabled for a servo:

- Servo automatically moves between `min_pos` and `max_pos`
- Moves by `step` degrees every 20ms
- Reverses direction at boundaries
- Continues until disabled or manual SET_POS command

**Example:** `SWEEP(servo=0, enable=1, min=30, max=150, step=3)`

- Servo sweeps between 30° and 150°
- Moves 3° every 20ms (150°/sec)
- Smooth back-and-forth motion

## Serial Output

The peripheral logs all operations to serial:

```text
=== CRUMBS Servo Controller Peripheral ===
I2C Address: 0x30
Type ID: 0x02
Servos: 2 (D9, D10)
WARNING: Use external 5V power for servos!

Servos initialized to 90 degrees
Servo controller peripheral ready!
Waiting for I2C commands...

SET_POS: Servo 0 target=45 (current=90, speed=0)
SET_SPEED: Servo 1 speed=5 (0=instant, 1–20=slow)
SWEEP: Servo 2 ENABLED (min=0, max=180, step=2)
```

## Troubleshooting

### Servo jitters or resets Arduino

- **Cause:** Insufficient power supply current
- **Fix:** Use external 5V PSU (2A+ recommended), ensure common ground

### Servo doesn't move

- **Check:** Signal wire connected to correct pin (D9/D10)
- **Check:** Servo power connected and external PSU turned on
- **Check:** Serial output for error messages

### Servo moves in wrong direction

- **Not an error:** Standard servo behavior varies by model
- **Fix:** Invert angles in your controller code if needed

## Notes

- All servos initialize to **90 degrees** on startup
- Position range: **0–180 degrees** (standard servo)
- Speed setting persists until changed
- Manual SET_POS automatically **disables sweep**
- Smooth movement updates every **20ms**
- Speed 0 = instant, Speed 20 = ~9°/sec (very slow)
