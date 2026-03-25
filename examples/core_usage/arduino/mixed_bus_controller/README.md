# Mixed Bus Controller (Arduino)

Arduino controller example for mixed-bus validation:

- CRUMBS candidate-address scanning
- raw register read from an off-the-shelf I2C sensor
- repeated-start path through `crumbs_arduino_write_then_read`

## What It Does

Each loop:

1. scans only configured CRUMBS candidate addresses,
2. reads one sensor register (`SENSOR_WHOAMI_REG`) using raw helper APIs,
3. prints results to Serial.

## Setup

Edit these constants in `mixed_bus_controller.ino`:

- `SENSOR_ADDR`
- `SENSOR_WHOAMI_REG`
- `kCrumbsCandidates[]`

## Expected Output

Serial should show:

- discovered CRUMBS addresses/types,
- `Sensor read rc=0 WHOAMI=0x..` for successful sensor reads.

