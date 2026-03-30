# Mixed Bus Lab Controller (Linux)

Lab-focused mixed-bus validation example for one Linux controller running all devices on one I2C bus:

1. CRUMBS slices: RLHT (`0x0A`), DCMT (`0x14`), DCMT (`0x15`)
2. EZO command/response sensors: pH (`0x63`), DO (`0x61`)
3. Bosch register sensor: BMP/BME280 (`0x76` or `0x77`)

This example is intentionally topology-specific so you can validate your full bench before provider-level mixed-bus tests.

## What It Does

Single pass, then exits:

1. Scans only the expected CRUMBS addresses with protocol-aware scan.
2. Sends a default `SET_REPLY` query to each discovered CRUMBS device and prints reply data.
3. Sends `R` command to EZO pH and DO sensors, waits, reads response frame, prints status + text payload.
4. Probes Bosch candidates (`0x76`, `0x77`) for chip ID register (`0xD0`) and prints one raw sample read (`0xF7`, 6 bytes).

Exit code is non-zero on validation failure.

## Build

From the CRUMBS repo root:

```bash
cmake --preset linux
cmake --build --preset linux
```

## Run

```bash
./build-linux/crumbs_mixed_bus_lab_controller /dev/i2c-1
```

Device path is optional; default is `/dev/i2c-1`.

## Pass Criteria

1. All expected CRUMBS addresses are found: `0x0A`, `0x14`, `0x15`.
2. CRUMBS default query/reply succeeds for discovered devices.
3. EZO pH and DO return status byte `0x01` (`SUCCESS`).
4. At least one Bosch sensor reports chip ID `0x58` (BMP280) or `0x60` (BME280).

## Notes

1. This example uses CRUMBS Linux HAL and raw I2C helper APIs only.
2. For typed EZO parsing/workflows, use `ezo-driver` examples separately.
3. If your lab addresses differ, edit constants at top of `main.c`.
