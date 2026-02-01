/**
 * @file config.h
 * @brief Configuration for LHWIT Manual Controller
 *
 * Edit these addresses to match your peripheral configurations.
 * Use i2cdetect or discovery_controller to find device addresses.
 */

#ifndef LHWIT_MANUAL_CONTROLLER_CONFIG_H
#define LHWIT_MANUAL_CONTROLLER_CONFIG_H

/* Peripheral I2C addresses (must match peripheral main.cpp definitions) */
#define CALCULATOR_ADDR 0x10
#define LED_ADDR 0x20
#define SERVO_ADDR 0x30

/* Default I2C bus device path */
#define DEFAULT_I2C_BUS_PATH "/dev/i2c-1"

#endif /* LHWIT_MANUAL_CONTROLLER_CONFIG_H */
