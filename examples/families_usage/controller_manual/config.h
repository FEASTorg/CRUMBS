#ifndef CONFIG_H
#define CONFIG_H

#include "../lhwit_family/calculator_ops.h"
#include "../lhwit_family/display_ops.h"
#include "../lhwit_family/led_ops.h"
#include "../lhwit_family/servo_ops.h"

/**
 * @file config.h
 * @brief Preconfigured device list for lhwit_family peripherals.
 *
 * Define devices as { type_id, address } pairs. Comment out unused devices.
 * Multiple devices of the same type are supported.
 *
 * Example configurations:
 *   - Standard: 1 calc, 1 led, 1 servo
 *   - LED-heavy: 3 LEDs, 1 servo (no calculator)
 *   - Testing: Just calculator and LED
 */

/* Device entry: { type_id, i2c_address } */
typedef struct
{
    uint8_t type_id;
    uint8_t addr;
} device_config_t;

/* Configured device list - edit as needed */
static const device_config_t DEVICE_CONFIG[] = {
    {CALC_TYPE_ID, 0x10},    /* Calculator at 0x10 */
    {LED_TYPE_ID, 0x20},     /* LED at 0x20 */
    {SERVO_TYPE_ID, 0x30},   /* Servo at 0x30 */
    {DISPLAY_TYPE_ID, 0x40}, /* Display at 0x40 */

    /* Examples of additional devices:
    { LED_TYPE_ID,   0x21 },  // Second LED at 0x21
    { LED_TYPE_ID,   0x22 },  // Third LED at 0x22
    { SERVO_TYPE_ID, 0x31 },  // Second servo at 0x31
    */
};

#define DEVICE_CONFIG_COUNT (sizeof(DEVICE_CONFIG) / sizeof(DEVICE_CONFIG[0]))

#endif /* CONFIG_H */
