/**
 * @file led_commands.h
 * @brief Example command definitions for an LED array peripheral.
 *
 * This file demonstrates the pattern for defining device commands.
 * Copy and modify this template for your own device types.
 *
 * Commands:
 * - LED_CMD_SET_ALL:   Set all LEDs using bitmask
 * - LED_CMD_SET_ONE:   Set single LED on/off
 * - LED_CMD_BLINK:     Blink all LEDs
 * - LED_CMD_GET_STATE: Request current state (via I2C read)
 */

#ifndef LED_COMMANDS_H
#define LED_COMMANDS_H

#include "crumbs.h"
#include "crumbs_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * LED Device Identity
 * ============================================================================ */

/** @brief Type ID for LED array device. */
#define LED_TYPE_ID         0x01

/* ============================================================================
 * Command Definitions
 * ============================================================================ */

/** @brief Set all LEDs. Payload: [bitmask:u8] */
#define LED_CMD_SET_ALL     0x01

/** @brief Set single LED. Payload: [index:u8, state:u8] */
#define LED_CMD_SET_ONE     0x02

/** @brief Blink all LEDs. Payload: [count:u8, delay_10ms:u8] */
#define LED_CMD_BLINK       0x03

/** @brief Request current state. Payload: none (reply has state) */
#define LED_CMD_GET_STATE   0x10

/* ============================================================================
 * Controller Side: Command Senders
 *
 * These functions build and send commands to an LED peripheral.
 * ============================================================================ */

/**
 * @brief Set all LEDs using bitmask (bit N = LED N).
 *
 * @param ctx      CRUMBS controller context.
 * @param addr     I2C address of LED peripheral.
 * @param write_fn I2C write function.
 * @param io       I2C context (Wire*, linux handle, etc.).
 * @param bitmask  LED state bitmask (bit 0 = LED 0, etc.).
 * @return 0 on success, non-zero on error.
 */
static inline int led_send_set_all(crumbs_context_t *ctx,
                                   uint8_t addr,
                                   crumbs_i2c_write_fn write_fn,
                                   void *io,
                                   uint8_t bitmask)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_SET_ALL);
    crumbs_msg_add_u8(&msg, bitmask);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Set single LED on or off.
 *
 * @param ctx      CRUMBS controller context.
 * @param addr     I2C address of LED peripheral.
 * @param write_fn I2C write function.
 * @param io       I2C context.
 * @param index    LED index (0-7).
 * @param state    0 = off, non-zero = on.
 * @return 0 on success, non-zero on error.
 */
static inline int led_send_set_one(crumbs_context_t *ctx,
                                   uint8_t addr,
                                   crumbs_i2c_write_fn write_fn,
                                   void *io,
                                   uint8_t index,
                                   uint8_t state)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_SET_ONE);
    crumbs_msg_add_u8(&msg, index);
    crumbs_msg_add_u8(&msg, state);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Blink all LEDs.
 *
 * @param ctx        CRUMBS controller context.
 * @param addr       I2C address of LED peripheral.
 * @param write_fn   I2C write function.
 * @param io         I2C context.
 * @param count      Number of blink cycles.
 * @param delay_10ms Delay between on/off in units of 10ms.
 * @return 0 on success, non-zero on error.
 */
static inline int led_send_blink(crumbs_context_t *ctx,
                                 uint8_t addr,
                                 crumbs_i2c_write_fn write_fn,
                                 void *io,
                                 uint8_t count,
                                 uint8_t delay_10ms)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_BLINK);
    crumbs_msg_add_u8(&msg, count);
    crumbs_msg_add_u8(&msg, delay_10ms);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Send GET_STATE command (peripheral will respond on next I2C read).
 *
 * @param ctx      CRUMBS controller context.
 * @param addr     I2C address of LED peripheral.
 * @param write_fn I2C write function.
 * @param io       I2C context.
 * @return 0 on success, non-zero on error.
 */
static inline int led_send_get_state(crumbs_context_t *ctx,
                                     uint8_t addr,
                                     crumbs_i2c_write_fn write_fn,
                                     void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_GET_STATE);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

#ifdef __cplusplus
}
#endif

#endif /* LED_COMMANDS_H */
