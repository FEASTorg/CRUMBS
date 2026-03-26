# Mixed Bus Controller (Linux)

Linux controller example for validating the new mixed-bus APIs:

- `crumbs_controller_scan_for_crumbs_candidates(...)`
- `crumbs_i2c_dev_read_reg_u8/u16be(...)`
- `crumbs_i2c_dev_write_reg_u8/u16be(...)`
- `crumbs_linux_write_then_read(...)`

This example is intended for **pre-release validation** on a real bus with:

1. one or more CRUMBS devices,
2. one off-the-shelf I2C sensor.

Recommended register-based reference sensor for validation:

- BMP280/BME280 at `0x76` (or `0x77`)
- chip-ID register `0xD0`
- expected ID `0x58` (BMP280) or `0x60` (BME280)
- Bosch datasheets:
  - https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
  - https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf

## Build

```bash
cmake -S . -B build -DCRUMBS_BUILD_IN_TREE=ON
cmake --build build
```

## Usage

```bash
./build/crumbs_mixed_bus_controller <i2c-dev> scan <candidate_csv> [strict|non-strict]
./build/crumbs_mixed_bus_controller <i2c-dev> read-u8 <sensor_addr> <reg_u8> <len> [repeat|norepeat]
./build/crumbs_mixed_bus_controller <i2c-dev> read-u16 <sensor_addr> <reg_u16> <len> [repeat|norepeat]
./build/crumbs_mixed_bus_controller <i2c-dev> read-ex <sensor_addr> <reg_csv> <len> [repeat|norepeat]
./build/crumbs_mixed_bus_controller <i2c-dev> write-u8 <sensor_addr> <reg_u8> <data_csv>
./build/crumbs_mixed_bus_controller <i2c-dev> write-u16 <sensor_addr> <reg_u16> <data_csv>
./build/crumbs_mixed_bus_controller <i2c-dev> write-ex <sensor_addr> <reg_csv> <data_csv>
```

## Examples

```bash
# Scan only known CRUMBS addresses
./build/crumbs_mixed_bus_controller /dev/i2c-1 scan 0x20,0x21,0x30 strict

# Read BMP/BME280 chip-ID register (u8 register address)
./build/crumbs_mixed_bus_controller /dev/i2c-1 read-u8 0x76 0xD0 1 repeat

# Read 6 bytes using u16 register address
./build/crumbs_mixed_bus_controller /dev/i2c-1 read-u16 0x40 0x0100 6 repeat

# Read 6 bytes from a 2-byte register preamble using generic -ex API
./build/crumbs_mixed_bus_controller /dev/i2c-1 read-ex 0x1E 0x20,0x08 6 repeat

# Write one config byte
./build/crumbs_mixed_bus_controller /dev/i2c-1 write-u8 0x76 0xF4 0x27

# Generic write with multi-byte register preamble
./build/crumbs_mixed_bus_controller /dev/i2c-1 write-ex 0x1E 0x20,0x09 0x10,0x00
```

## Validation Checklist

- `scan` only touches candidate addresses you pass in.
- CRUMBS devices are discovered with address + type.
- sensor read/write works for both u8 and u16 register-address forms.
- generic `read-ex`/`write-ex` works for sensors with custom register preambles.
- repeated-start mode works for sensors that require it.
