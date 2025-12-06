/**
 * @file led_commands.h
 * @brief Command definitions for LED array peripheral.
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
 * ============================================================================ */

/**
 * @brief Set all LEDs using bitmask (bit N = LED N).
 */
static inline int led_cmd_set_all(crumbs_context_t *ctx, uint8_t addr,
                                   crumbs_i2c_write_fn write_fn, void *io,
                                   uint8_t bitmask)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_SET_ALL);
    crumbs_msg_add_u8(&msg, bitmask);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Set a single LED on or off.
 */
static inline int led_cmd_set_one(crumbs_context_t *ctx, uint8_t addr,
                                   crumbs_i2c_write_fn write_fn, void *io,
                                   uint8_t index, uint8_t state)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_SET_ONE);
    crumbs_msg_add_u8(&msg, index);
    crumbs_msg_add_u8(&msg, state);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Blink all LEDs.
 */
static inline int led_cmd_blink(crumbs_context_t *ctx, uint8_t addr,
                                 crumbs_i2c_write_fn write_fn, void *io,
                                 uint8_t count, uint8_t delay_10ms)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_BLINK);
    crumbs_msg_add_u8(&msg, count);
    crumbs_msg_add_u8(&msg, delay_10ms);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Request LED state (send empty command, then read response).
 */
static inline int led_cmd_get_state(crumbs_context_t *ctx, uint8_t addr,
                                     crumbs_i2c_write_fn write_fn, void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, LED_TYPE_ID, LED_CMD_GET_STATE);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

#ifdef __cplusplus
}
#endif

#endif /* LED_COMMANDS_H */
