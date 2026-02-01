/**
 * @file servo_ops.h
 * @brief Servo control command definitions (Type ID 0x02)
 *
 * This file defines commands for controlling a 2-servo peripheral.
 * The peripheral controls servo motors (D9-D10) with position control,
 * speed limiting, and sweep patterns.
 *
 * Pattern: Position-control interface
 * - SET operations (0x01-0x03): Control servo positions and sweep modes
 * - GET operations (0x80-0x81): Query current positions and settings
 *
 * Commands:
 * - SERVO_OP_SET_POS:    Set servo position immediately
 * - SERVO_OP_SET_SPEED:  Set servo movement speed limit
 * - SERVO_OP_SWEEP:      Configure sweep pattern
 * - SERVO_OP_GET_POS:    Request current positions (via SET_REPLY)
 * - SERVO_OP_GET_SPEED:  Request speed limits (via SET_REPLY)
 */

#ifndef SERVO_OPS_H
#define SERVO_OPS_H

#include "crumbs.h"
#include "crumbs_message_helpers.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Device Identity
 * ============================================================================ */

/** @brief Type ID for servo controller device. */
#define SERVO_TYPE_ID 0x02

/* Module protocol version (per versioning.md convention) */
#define SERVO_MODULE_VER_MAJOR 1
#define SERVO_MODULE_VER_MINOR 0
#define SERVO_MODULE_VER_PATCH 0

/* ============================================================================
 * Command Definitions: SET Operations (Control Servos)
 * ============================================================================ */

/**
 * @brief Set servo position immediately.
 * Payload: [servo_idx:u8][position:u8]
 *   - servo_idx: Servo index (0-1 for D9-D10)
 *   - position: Target position (0-180 degrees)
 */
#define SERVO_OP_SET_POS 0x01

/**
 * @brief Set servo movement speed limit.
 * Payload: [servo_idx:u8][speed:u8]
 *   - servo_idx: Servo index (0-1)
 *   - speed: Degrees per update (0=instant, 1-20=limited speed)
 */
#define SERVO_OP_SET_SPEED 0x02

/**
 * @brief Configure servo sweep pattern.
 * Payload: [servo_idx:u8][enable:u8][min_pos:u8][max_pos:u8][step:u8]
 *   - servo_idx: Servo index (0-1)
 *   - enable: 0=disable sweep, 1=enable sweep
 *   - min_pos: Minimum position (0-180 degrees)
 *   - max_pos: Maximum position (0-180 degrees)
 *   - step: Degrees to move per sweep update
 */
#define SERVO_OP_SWEEP 0x03

/* ============================================================================
 * Command Definitions: GET Operations (Query State via SET_REPLY)
 * ============================================================================ */

/**
 * @brief Request current servo positions.
 * Payload: none
 * Reply: [pos0:u8][pos1:u8] (current positions in degrees)
 */
#define SERVO_OP_GET_POS 0x80

/**
 * @brief Request servo speed limits.
 * Payload: none
 * Reply: [speed0:u8][speed1:u8] (speed limits in degrees/update)
 */
#define SERVO_OP_GET_SPEED 0x81

    /* ============================================================================
     * Controller Side: Command Senders
     * ============================================================================ */

    /**
     * @brief Set servo position.
     *
     * @param ctx       CRUMBS controller context.
     * @param addr      I2C address of servo peripheral.
     * @param write_fn  I2C write function.
     * @param io        I2C context (Wire*, linux handle, etc.).
     * @param servo_idx Servo index (0-1).
     * @param position  Target position (0-180 degrees).
     * @return 0 on success, non-zero on error.
     */
    static inline int servo_send_set_pos(crumbs_context_t *ctx,
                                         uint8_t addr,
                                         crumbs_i2c_write_fn write_fn,
                                         void *io,
                                         uint8_t servo_idx,
                                         uint8_t position)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_OP_SET_POS);
        crumbs_msg_add_u8(&msg, servo_idx);
        crumbs_msg_add_u8(&msg, position);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Set servo movement speed limit.
     *
     * @param ctx       CRUMBS controller context.
     * @param addr      I2C address of servo peripheral.
     * @param write_fn  I2C write function.
     * @param io        I2C context.
     * @param servo_idx Servo index (0-1).
     * @param speed     Speed limit (0=instant, 1-20=limited).
     * @return 0 on success, non-zero on error.
     */
    static inline int servo_send_set_speed(crumbs_context_t *ctx,
                                           uint8_t addr,
                                           crumbs_i2c_write_fn write_fn,
                                           void *io,
                                           uint8_t servo_idx,
                                           uint8_t speed)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_OP_SET_SPEED);
        crumbs_msg_add_u8(&msg, servo_idx);
        crumbs_msg_add_u8(&msg, speed);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Configure servo sweep pattern.
     *
     * @param ctx       CRUMBS controller context.
     * @param addr      I2C address of servo peripheral.
     * @param write_fn  I2C write function.
     * @param io        I2C context.
     * @param servo_idx Servo index (0-1).
     * @param enable    Enable sweep (0=OFF, 1=ON).
     * @param min_pos   Minimum position (0-180 degrees).
     * @param max_pos   Maximum position (0-180 degrees).
     * @param step      Step size (degrees per update).
     * @return 0 on success, non-zero on error.
     */
    static inline int servo_send_sweep(crumbs_context_t *ctx,
                                       uint8_t addr,
                                       crumbs_i2c_write_fn write_fn,
                                       void *io,
                                       uint8_t servo_idx,
                                       uint8_t enable,
                                       uint8_t min_pos,
                                       uint8_t max_pos,
                                       uint8_t step)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_OP_SWEEP);
        crumbs_msg_add_u8(&msg, servo_idx);
        crumbs_msg_add_u8(&msg, enable);
        crumbs_msg_add_u8(&msg, min_pos);
        crumbs_msg_add_u8(&msg, max_pos);
        crumbs_msg_add_u8(&msg, step);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Query current servo positions (peripheral will respond on next I2C read).
     *
     * Uses SET_REPLY pattern (0xFE) to request servo positions.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of servo peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int servo_query_pos(crumbs_context_t *ctx,
                                      uint8_t addr,
                                      crumbs_i2c_write_fn write_fn,
                                      void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, 0, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&msg, SERVO_OP_GET_POS);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Query servo speed limits (peripheral will respond on next I2C read).
     *
     * Uses SET_REPLY pattern (0xFE) to request speed limits.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of servo peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int servo_query_speed(crumbs_context_t *ctx,
                                        uint8_t addr,
                                        crumbs_i2c_write_fn write_fn,
                                        void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, 0, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&msg, SERVO_OP_GET_SPEED);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

#ifdef __cplusplus
}
#endif

#endif /* SERVO_OPS_H */
