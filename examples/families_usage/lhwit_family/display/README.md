# LHWIT Display Peripheral

**Type ID:** `0x04` (DISPLAY_TYPE_ID)  
**Default Address:** `0x40`  
**Hardware:** Arduino Nano + 5641AS Quad 7-Segment Display

## Overview

The Display peripheral provides control of a 4-digit 7-segment display (5641AS or compatible). It supports displaying numbers with decimal points, custom segment patterns, brightness control, and querying the current displayed value.

## Hardware Setup

### Components
- Arduino Nano (ATmega328P)
- 5641AS quad 7-segment display (common cathode)
- Jumper wires

### Wiring

**Segment Pins (a-g, dp) → Arduino Digital Pins:**
```
Segment  Arduino Pin
-------------------
   a        D9
   b        D13
   c        D4
   d        D6
   e        D7
   f        D10
   g        D3
   dp       D5
```

**Digit Select Pins → Arduino Digital Pins:**
```
Digit    Arduino Pin
--------------------
 D1         D8
 D2         D11
 D3         D12
 D4         D2
```

**I2C Pins:**
```
I2C Pin   Arduino Pin
---------------------
 SDA        A4
 SCL        A5
```

### 5641AS Pin Mapping

The 5641AS has the following pinout (view from component side):
```
         11 7  4  2  1 10  5
         ┌──────────────────┐
         │ 5641AS Quad 7seg │
         └──────────────────┘
          6 12 15 3 13 9  14 8

Pin    Function
-----------------
1      e (segment)
2      d (segment)
3      dp (decimal point)
4      c (segment)
5      g (segment)
6      D4 (digit 4 select)
7      b (segment)
8      D3 (digit 3 select)
9      D2 (digit 2 select)
10     f (segment)
11     a (segment)
12     D1 (digit 1 select)
13     (not connected)
14     (not connected)
15     (not connected)
```

## Operations

### SET Commands

| Opcode | Name              | Payload                         | Description                            |
| ------ | ----------------- | ------------------------------- | -------------------------------------- |
| 0x01   | SET_NUMBER        | [number:u16][decimal_pos:u8]    | Display number with decimal point      |
| 0x02   | SET_SEGMENTS      | [d0:u8][d1:u8][d2:u8][d3:u8]    | Set custom segment patterns            |
| 0x03   | SET_BRIGHTNESS    | [level:u8]                      | Set brightness (0-10)                  |
| 0x04   | CLEAR             | none                            | Clear display                          |

### GET Commands (via SET_REPLY)

| Opcode | Name       | Reply Payload                            | Description           |
| ------ | ---------- | ---------------------------------------- | --------------------- |
| 0x80   | GET_VALUE  | [number:u16][decimal:u8][brightness:u8]  | Current display state |

## Usage Examples

### Display Number
```
> display 0 set_number 1234 0
Displays: 1234 (no decimal)

> display 0 set_number 1234 3
Displays: 123.4 (decimal on digit 3)

> display 0 set_number 1234 2
Displays: 12.34 (decimal on digit 2)

> display 0 set_number 42 0
Displays:   42 (right-aligned)
```

**Decimal Position:**
- 0 = no decimal
- 1 = decimal on leftmost digit (D1)
- 2 = decimal on digit 2
- 3 = decimal on digit 3
- 4 = decimal on rightmost digit (D4)

### Custom Segments
```
> display 0 set_segments 0x7E 0x30 0x6D 0x79
Displays custom patterns (e.g., "HELP" or similar)
```

Segment bit mapping:
```
Bit 7 ⇒ 0:  [a][b][c][d][e][f][g][dot]

        a (bit 7)
     f     b (bit 6)
        g (bit 1)
     e     c (bit 5)
        d (bit 4)  dot (bit 0)

Example: 0x7E = 0b01111110 = segments b,c,d,e,f,g ON = digit "0"
```

### Brightness Control
```
> display 0 set_brightness 5
Sets brightness to medium (5/10)
```

### Clear Display
```
> display 0 clear
Clears all segments
```

### Query Current Value
```
> display 0 get_value
Returns: number=1234, decimal=2, brightness=5
```

## Building & Uploading

```bash
cd examples/families_usage/lhwit_family/display
pio run -t upload
```

Or specify the upload port:
```bash
pio run -t upload --upload-port /dev/ttyUSB0   # Linux
pio run -t upload --upload-port COM5           # Windows
```

## Testing

### Monitor Serial Output
```bash
pio device monitor
```

### Test with Controller
From the discovery or manual controller:
```
> scan
> display 0 set_number 1234 0
> display 0 get_value
```

## Implementation Notes

### Display Multiplexing
The 7-segment display requires continuous multiplexing to show all 4 digits. The `refresh_display()` function is called frequently from `loop()` to cycle through digits at ~2ms intervals, creating the illusion of all digits being lit simultaneously.

### Brightness Control
The current implementation stores the brightness value but doesn't actively control it (Simple5641AS library limitation). For hardware PWM control, you would need to add PWM to the digit select pins.

### Number Display Format
- Numbers 0-9999 are supported
- Numbers < 1000 are right-aligned with leading spaces
- Decimal position: 0=none, 1-4=after digit 1-4

### Custom Segments
Advanced users can set individual segments to create custom characters or symbols using `SET_SEGMENTS`. Each byte controls 8 segments (7 + decimal point).

## Troubleshooting

**Display is blank:**
- Check wiring, especially digit select pins (D10-D13)
- Verify I2C address (default 0x40)
- Check serial monitor for boot messages

**Display flickers:**
- Normal during message processing
- If persistent, check `refresh_display()` timing

**Incorrect segments lit:**
- Verify segment pin mapping matches your display
- Different 7-segment displays may have different pinouts
- Consult your display's datasheet

## References

- [display_ops.h](../display_ops.h) - Operation definitions
- [lhwit_ops.h](../lhwit_ops.h) - Family-wide helpers
- [Simple5641AS Library](https://github.com/adrian200223/Simple5641AS) - Display driver
