#pragma once

#include <stdint.h>

static const uint32_t SERIAL_BAUD = 115200;
static const uint32_t HEARTBEAT_INTERVAL_MS = 500;
static const uint32_t SERIAL_WAIT_MS = 10;

static const uint32_t REQUEST_READ_DELAY_MS = 50;
static const uint8_t SCAN_START_ADDR = 0x03;
static const uint8_t SCAN_END_ADDR = 0x77;
static const uint32_t SCAN_TIMEOUT_US = 50000;
