# LED Controller Peripheral

4-channel LED controller peripheral demonstrating state-query interface pattern.

## Hardware

- **Board:** Arduino Nano (ATmega328P)
- **I2C Address:** 0x20
- **I2C Pins:** A4 (SDA), A5 (SCL)
- **Type ID:** 0x01
- **LED Pins:**
  - LED 0: Pin 4 (D4)
  - LED 1: Pin 5 (D5)
  - LED 2: Pin 6 (D6)
  - LED 3: Pin 7 (D7)

## Features

- **4 Independent LEDs:** Control each LED individually or all at once
- **Static Control:** Turn LEDs on/off
- **Blink Patterns:** Configure per-LED blinking with custom periods
- **State Query:** Read back current LED states and blink configurations

## Building

### PlatformIO

```bash
cd led
pio run                    # Build
pio run -t upload          # Upload to Arduino Nano
pio device monitor         # View serial output
```

## Wiring

Each LED requires a current-limiting resistor (220Ω recommended):

```
Arduino Pin → 220Ω Resistor → LED Anode (+) → LED Cathode (-) → GND
```

| LED Index | Physical Pin | Arduino Name |
| --------- | ------------ | ------------ |
| 0         | 4            | D4           |
| 1         | 5            | D5           |
| 2         | 6            | D6           |
| 3         | 7            | D7           |

## Operations

### SET Commands (Control LEDs)

| Opcode | Command | Payload                        | Description                                 |
| ------ | ------- | ------------------------------ | ------------------------------------------- |
| 0x01   | SET_ALL | [mask:u8]                      | Set all LEDs (bit 0=LED0, bit 1=LED1, etc.) |
| 0x02   | SET_ONE | [idx:u8][state:u8]             | Set single LED (idx=0-3, state=0/1)         |
| 0x03   | BLINK   | [idx:u8][en:u8][period_ms:u16] | Configure LED blinking                      |

### GET Commands (Query State via SET_REPLY)

| Opcode | Command   | Reply                                                         | Description                   |
| ------ | --------- | ------------------------------------------------------------- | ----------------------------- |
| 0x80   | GET_STATE | [mask:u8]                                                     | Current LED states (bit mask) |
| 0x81   | GET_BLINK | [led0_en:u8][led0_period:u16]...[led3_en:u8][led3_period:u16] | Blink config (12 bytes)       |

## Example Usage

From a Linux controller:

```c
#include "led_ops.h"

/* Turn all LEDs on */
led_send_set_all(&ctx, 0x20, write_fn, io, 0x0F);  // 0b1111

/* Turn LED 2 on, others unchanged */
led_send_set_one(&ctx, 0x20, write_fn, io, 2, 1);

/* Configure LED 1 to blink at 500ms period */
led_send_blink(&ctx, 0x20, write_fn, io, 1, 1, 500);

/* Query current LED states */
led_query_get_state(&ctx, 0x20, write_fn, io);
uint8_t states;
/* ...read response and parse states... */
```

## LED State Bitmask

The `SET_ALL` command and `GET_STATE` response use a bitmask:

```
Bit:  7  6  5  4  3  2  1  0
LED:  -  -  -  - L3 L2 L1 L0
```

**Examples:**

- `0x00` (0b0000) = All LEDs off
- `0x0F` (0b1111) = All LEDs on
- `0x05` (0b0101) = LED0 and LED2 on, LED1 and LED3 off
- `0x0A` (0b1010) = LED1 and LED3 on, LED0 and LED2 off

## Blink Configuration

Blink configuration is per-LED with independent periods:

```c
/* Blink LED 0 slowly (1 second period) */
led_send_blink(&ctx, 0x20, write_fn, io, 0, 1, 1000);

/* Blink LED 1 quickly (200ms period) */
led_send_blink(&ctx, 0x20, write_fn, io, 1, 1, 200);

/* Disable blink on LED 2 */
led_send_blink(&ctx, 0x20, write_fn, io, 2, 0, 0);
```

When blink is enabled:

- LED toggles on/off at specified period
- Period is full cycle (on + off time)
- Minimum period: 50ms (practical limit)

## Serial Output

The peripheral logs all operations to serial:

```
=== CRUMBS LED Controller Peripheral ===
I2C Address: 0x20
Type ID: 0x01
LEDs: 4 (D4, D5, D18, D19)

LED controller peripheral ready!
Waiting for I2C commands...

SET_ALL: mask=0x0F (all on)
SET_ONE: LED 2 state=1
BLINK: LED 1 enabled (period=500ms)
```

## Notes

- All LEDs initialize to **OFF** on startup
- Blink state is independent of static on/off commands
- Setting an LED with `SET_ONE` or `SET_ALL` does **not** disable its blink
- LEDs blink in the background while peripheral handles I2C commands
- Blink period is the **full cycle** time (on → off → on)
- Use appropriate current-limiting resistors (220Ω for 5V Arduino pins)
