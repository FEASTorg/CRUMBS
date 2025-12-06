/**
 * @file servo_commands.h
 * @brief Command definitions for servo controller peripheral.
 *
 * Commands:
 * - SERVO_CMD_SET_ANGLE:  Set single servo angle
 * - SERVO_CMD_SET_BOTH:   Set both servos at once
 * - SERVO_CMD_SWEEP:      Sweep servo from start to end
 * - SERVO_CMD_CENTER_ALL: Center all servos to 90°
 * - SERVO_CMD_GET_ANGLES: Request current angles (via I2C read)
 */

#ifndef SERVO_COMMANDS_H
#define SERVO_COMMANDS_H

#include "crumbs.h"
#include "crumbs_msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Servo Device Identity
 * ============================================================================ */

/** @brief Type ID for servo controller device. */
#define SERVO_TYPE_ID         0x02

/* ============================================================================
 * Command Definitions
 * ============================================================================ */

/** @brief Set single servo. Payload: [channel:u8, angle:u8] */
#define SERVO_CMD_SET_ANGLE   0x01

/** @brief Set both servos. Payload: [angle0:u8, angle1:u8] */
#define SERVO_CMD_SET_BOTH    0x02

/** @brief Sweep servo. Payload: [channel:u8, start:u8, end:u8, step_ms:u8] */
#define SERVO_CMD_SWEEP       0x03

/** @brief Center all servos. Payload: none */
#define SERVO_CMD_CENTER_ALL  0x04

/** @brief Request current angles. Payload: none (reply has angles) */
#define SERVO_CMD_GET_ANGLES  0x10

/* ============================================================================
 * Controller Side: Command Senders
 * ============================================================================ */

/**
 * @brief Set a single servo to specified angle.
 */
static inline int servo_cmd_set_angle(crumbs_context_t *ctx, uint8_t addr,
                                       crumbs_i2c_write_fn write_fn, void *io,
                                       uint8_t channel, uint8_t angle)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_SET_ANGLE);
    crumbs_msg_add_u8(&msg, channel);
    crumbs_msg_add_u8(&msg, angle);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Set both servos at once.
 */
static inline int servo_cmd_set_both(crumbs_context_t *ctx, uint8_t addr,
                                      crumbs_i2c_write_fn write_fn, void *io,
                                      uint8_t angle0, uint8_t angle1)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_SET_BOTH);
    crumbs_msg_add_u8(&msg, angle0);
    crumbs_msg_add_u8(&msg, angle1);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Sweep servo from start to end angle.
 */
static inline int servo_cmd_sweep(crumbs_context_t *ctx, uint8_t addr,
                                   crumbs_i2c_write_fn write_fn, void *io,
                                   uint8_t channel, uint8_t start,
                                   uint8_t end, uint8_t step_ms)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_SWEEP);
    crumbs_msg_add_u8(&msg, channel);
    crumbs_msg_add_u8(&msg, start);
    crumbs_msg_add_u8(&msg, end);
    crumbs_msg_add_u8(&msg, step_ms);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Center all servos to 90°.
 */
static inline int servo_cmd_center_all(crumbs_context_t *ctx, uint8_t addr,
                                        crumbs_i2c_write_fn write_fn, void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_CENTER_ALL);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Request servo angles (send command, then read response).
 */
static inline int servo_cmd_get_angles(crumbs_context_t *ctx, uint8_t addr,
                                        crumbs_i2c_write_fn write_fn, void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_GET_ANGLES);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

#ifdef __cplusplus
}
#endif

#endif /* SERVO_COMMANDS_H */
