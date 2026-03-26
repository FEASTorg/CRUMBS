# Mixed Bus Controller (Arduino)

Arduino controller example for mixed-bus validation:

- CRUMBS candidate-address scanning
- raw register read from an off-the-shelf I2C sensor (BMP/BME280)
- repeated-start path through `crumbs_arduino_write_then_read`

## What It Does

Each loop:

1. scans only configured CRUMBS candidate addresses,
2. reads one sensor register (`SENSOR_CHIP_ID_REG`) using raw helper APIs,
3. prints results to Serial.

Default target sensor profile:

- address: `0x76` (common breakout default; some boards use `0x77`)
- register: `0xD0` (chip ID)
- expected value: `0x58` (BMP280) or `0x60` (BME280)

## Setup

Edit `mixed_bus_config.h`:

- `SENSOR_ADDR`
- `SENSOR_CHIP_ID_REG`
- `kCrumbsCandidates[]`

## Expected Output

Serial should show:

- discovered CRUMBS addresses/types,
- `Sensor read rc=0 CHIP_ID=0x58 (BMP280)` or `CHIP_ID=0x60 (BME280)`.

## Reference (Primary Sources)

- BMP280 datasheet (Bosch): https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf
- BME280 datasheet (Bosch): https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf
