/**
 * @file led_ops.h
 * @brief LED control command definitions (Type ID 0x01)
 *
 * This file defines commands for controlling a 4-LED array peripheral.
 * The peripheral controls individual LEDs (D4-D7) with support for
 * static state and blinking patterns.
 *
 * Pattern: State-query interface
 * - SET operations (0x01-0x03): Set LED states and blink patterns
 * - GET operations (0x80-0x81): Query current state via SET_REPLY
 *
 * Commands:
 * - LED_OP_SET_ALL:     Set all 4 LEDs at once
 * - LED_OP_SET_ONE:     Set individual LED state
 * - LED_OP_BLINK:       Configure LED blinking
 * - LED_OP_GET_STATE:   Request LED states (via SET_REPLY)
 * - LED_OP_GET_BLINK:   Request blink configuration (via SET_REPLY)
 */

#ifndef LED_OPS_H
#define LED_OPS_H

#include "crumbs.h"
#include "crumbs_message_helpers.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Device Identity
 * ============================================================================ */

/** @brief Type ID for LED array device. */
#define LED_TYPE_ID 0x01

/* ============================================================================
 * Command Definitions: SET Operations (Control LEDs)
 * ============================================================================ */

/**
 * @brief Set all LEDs at once.
 * Payload: [mask:u8] (bits 0-3 control LEDs 0-3, bit=1 means ON)
 * Example: 0x05 = 0b0101 = LEDs 0 and 2 ON, LEDs 1 and 3 OFF
 */
#define LED_OP_SET_ALL 0x01

/**
 * @brief Set individual LED state.
 * Payload: [led_idx:u8][state:u8]
 *   - led_idx: LED index (0-3 for D4-D7)
 *   - state: 0=OFF, 1=ON
 */
#define LED_OP_SET_ONE 0x02

/**
 * @brief Configure LED blinking.
 * Payload: [led_idx:u8][enable:u8][period_ms:u16]
 *   - led_idx: LED index (0-3)
 *   - enable: 0=disable blink, 1=enable blink
 *   - period_ms: Blink period in milliseconds (full on-off cycle)
 */
#define LED_OP_BLINK 0x03

/* ============================================================================
 * Command Definitions: GET Operations (Query State via SET_REPLY)
 * ============================================================================ */

/**
 * @brief Request current LED states.
 * Payload: none
 * Reply: [states:u8] (bits 0-3 represent LEDs 0-3, bit=1 means ON)
 */
#define LED_OP_GET_STATE 0x80

/**
 * @brief Request blink configuration for all LEDs.
 * Payload: none
 * Reply: [led0_enable:u8][led0_period:u16]...[led3_enable:u8][led3_period:u16]
 *        Total: 12 bytes (3 bytes per LED × 4 LEDs)
 */
#define LED_OP_GET_BLINK 0x81

    /* ============================================================================
     * Controller Side: Command Senders
     * ============================================================================ */

    /**
     * @brief Set all LEDs at once.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of LED peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context (Wire*, linux handle, etc.).
     * @param mask     LED mask (bits 0-3 control LEDs 0-3).
     * @return 0 on success, non-zero on error.
     */
    static inline int led_send_set_all(crumbs_context_t *ctx,
                                       uint8_t addr,
                                       crumbs_i2c_write_fn write_fn,
                                       void *io,
                                       uint8_t mask)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, LED_TYPE_ID, LED_OP_SET_ALL);
        crumbs_msg_add_u8(&msg, mask);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Set individual LED state.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of LED peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @param led_idx  LED index (0-3).
     * @param state    LED state (0=OFF, 1=ON).
     * @return 0 on success, non-zero on error.
     */
    static inline int led_send_set_one(crumbs_context_t *ctx,
                                       uint8_t addr,
                                       crumbs_i2c_write_fn write_fn,
                                       void *io,
                                       uint8_t led_idx,
                                       uint8_t state)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, LED_TYPE_ID, LED_OP_SET_ONE);
        crumbs_msg_add_u8(&msg, led_idx);
        crumbs_msg_add_u8(&msg, state);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Configure LED blinking.
     *
     * @param ctx       CRUMBS controller context.
     * @param addr      I2C address of LED peripheral.
     * @param write_fn  I2C write function.
     * @param io        I2C context.
     * @param led_idx   LED index (0-3).
     * @param enable    Enable blinking (0=OFF, 1=ON).
     * @param period_ms Blink period in milliseconds.
     * @return 0 on success, non-zero on error.
     */
    static inline int led_send_blink(crumbs_context_t *ctx,
                                     uint8_t addr,
                                     crumbs_i2c_write_fn write_fn,
                                     void *io,
                                     uint8_t led_idx,
                                     uint8_t enable,
                                     uint16_t period_ms)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, LED_TYPE_ID, LED_OP_BLINK);
        crumbs_msg_add_u8(&msg, led_idx);
        crumbs_msg_add_u8(&msg, enable);
        crumbs_msg_add_u16(&msg, period_ms);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Query current LED states (peripheral will respond on next I2C read).
     *
     * Uses SET_REPLY pattern (0xFE) to request LED states.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of LED peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int led_query_state(crumbs_context_t *ctx,
                                      uint8_t addr,
                                      crumbs_i2c_write_fn write_fn,
                                      void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, 0, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&msg, LED_OP_GET_STATE);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Query blink configuration (peripheral will respond on next I2C read).
     *
     * Uses SET_REPLY pattern (0xFE) to request blink configuration.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of LED peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int led_query_blink(crumbs_context_t *ctx,
                                      uint8_t addr,
                                      crumbs_i2c_write_fn write_fn,
                                      void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, 0, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&msg, LED_OP_GET_BLINK);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

#ifdef __cplusplus
}
#endif

#endif /* LED_OPS_H */
