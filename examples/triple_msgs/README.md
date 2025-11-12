# Triple Messages Example

Minimal three-board demo for CRUMBS: `controller_example` issues SEND/REQUEST commands, while each `peripheral_example` slice drives three LEDs and reports its state.

## Rig at a Glance

- 3× Arduino Nano (ATmega328P)
- 3× SSD1306 128×64 OLEDs on software I²C pins D7/D8
- LEDs on D4 (green), D6 (yellow), D5 (red) with 220 Ω resistors
- Pull-ups present only on the controller; anything in the 2–5 kΩ range works
- Common SDA/SCL on A4/A5 and a shared ground between all boards

```text
Controller Nano                          Peripheral Nano(s)
--------------                          -------------------
D7 → OLED SDA                             D7 → OLED SDA
D8 → OLED SCL                             D8 → OLED SCL
D4/D6/D5 → LEDs → resistors → GND         D4/D6/D5 → LEDs → resistors → GND
A4 ↔ bus SDA                              A4 ↔ bus SDA
A5 ↔ bus SCL                              A5 ↔ bus SCL
Pull-ups only here                        (no extra pull-ups)
```

## Flashing the Sketches

1. Install **CRUMBS**, **U8g2**, and **AceCRC (v1.1.1)**.
2. Set `kSliceI2cAddress` per slice (e.g. `0x08`, `0x09`) in `peripheral_example.ino`.
3. Upload `controller_example` to the controller Nano and the slice sketch (with its address) to each peripheral Nano.

## Serial Commands (115200 baud)

```text
SEND <addr> <green> <yellow> <red> <period_ms>
REQUEST <addr>
```

- Ratios are 0.0–1.0; `period_ms` sets the shared blink cycle.
- Hex addresses (`0x08`) or decimal (`8`) are both accepted.
- Controller OLED lines: status, last TX/RX with CRC, current LED ratios.

## Slice Behaviour

- `SEND` updates internal LED ratios and immediately applies them without blocking.
- `REQUEST` returns the stored ratios, the active period, uptime seconds, and a CRC byte.
- Slice OLED shows status, last command, LED ratios, and the CRC of the last response.

## Quick Sanity Check

1. Power all boards: both OLEDs should show CRUMBS banners.
2. `SEND 8 0.5 0 0.25 1500` – controller yellow LED blips, slice LEDs match the ratios.
3. `REQUEST 8` – controller prints the reply and shows an `RX` line; slice logs “response sent”.
4. Repeat for the second slice address.

If a transfer fails, the controller flips to red, prints the Wire error code, and the OLED marks the last exchange as `FAIL`. Recheck wiring, addresses, or long cable runs introducing noise.

That’s the whole loop: a tiny CRUMBS controller driving slices and asking them how they’re doing. Hack on it from here.
