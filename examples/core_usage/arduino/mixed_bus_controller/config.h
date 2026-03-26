#pragma once

#include <stdint.h>

static const uint32_t SERIAL_BAUD = 115200;
static const uint32_t HEARTBEAT_INTERVAL_MS = 500;
static const uint32_t SENSOR_READ_TIMEOUT_US = 10000;
static const uint32_t LOOP_DELAY_MS = 2000;

// Bosch BMP280/BME280 register-based reference sensor.
// Address selection from datasheet:
// - SDO=GND   -> 0x76
// - SDO=VDDIO -> 0x77
static const uint8_t SENSOR_ADDR = 0x76;

// Chip-ID register and expected production values from Bosch datasheets.
static const uint8_t SENSOR_CHIP_ID_REG = 0xD0;
static const uint8_t BMP280_CHIP_ID = 0x58;
static const uint8_t BME280_CHIP_ID = 0x60;

// Probe only known CRUMBS addresses on mixed buses.
static const uint8_t kCrumbsCandidates[] = {0x20, 0x21, 0x30};
