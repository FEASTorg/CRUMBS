/**
 * @file servo_commands.h
 * @brief Example command definitions for a servo controller peripheral.
 *
 * This file demonstrates the pattern for defining device commands.
 * Copy and modify this template for your own device types.
 *
 * Commands:
 * - SERVO_CMD_SET_ANGLE:  Set single servo angle
 * - SERVO_CMD_SET_BOTH:   Set both servo angles at once
 * - SERVO_CMD_SWEEP:      Sweep a servo between angles
 * - SERVO_CMD_CENTER_ALL: Center all servos to 90 degrees
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
#define SERVO_TYPE_ID       0x02

/* ============================================================================
 * Command Definitions
 * ============================================================================ */

/** @brief Set single servo angle. Payload: [channel:u8, angle:u8] */
#define SERVO_CMD_SET_ANGLE     0x01

/** @brief Set both servos. Payload: [angle0:u8, angle1:u8] */
#define SERVO_CMD_SET_BOTH      0x02

/** @brief Sweep servo. Payload: [channel:u8, start:u8, end:u8, step_ms:u8] */
#define SERVO_CMD_SWEEP         0x03

/** @brief Center all servos to 90 degrees. Payload: none */
#define SERVO_CMD_CENTER_ALL    0x04

/** @brief Request current angles. Payload: none (reply has angles) */
#define SERVO_CMD_GET_ANGLES    0x10

/* ============================================================================
 * Controller Side: Command Senders
 *
 * These functions build and send commands to a servo peripheral.
 * ============================================================================ */

/**
 * @brief Set single servo angle (0-180 degrees).
 *
 * @param ctx      CRUMBS controller context.
 * @param addr     I2C address of servo peripheral.
 * @param write_fn I2C write function.
 * @param io       I2C context (Wire*, linux handle, etc.).
 * @param channel  Servo channel (0 or 1).
 * @param angle    Angle in degrees (0-180).
 * @return 0 on success, non-zero on error.
 */
static inline int servo_send_angle(crumbs_context_t *ctx,
                                   uint8_t addr,
                                   crumbs_i2c_write_fn write_fn,
                                   void *io,
                                   uint8_t channel,
                                   uint8_t angle)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_SET_ANGLE);
    crumbs_msg_add_u8(&msg, channel);
    crumbs_msg_add_u8(&msg, angle);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Set both servo angles at once.
 *
 * @param ctx      CRUMBS controller context.
 * @param addr     I2C address of servo peripheral.
 * @param write_fn I2C write function.
 * @param io       I2C context.
 * @param angle0   Angle for channel 0 (0-180).
 * @param angle1   Angle for channel 1 (0-180).
 * @return 0 on success, non-zero on error.
 */
static inline int servo_send_both(crumbs_context_t *ctx,
                                  uint8_t addr,
                                  crumbs_i2c_write_fn write_fn,
                                  void *io,
                                  uint8_t angle0,
                                  uint8_t angle1)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_SET_BOTH);
    crumbs_msg_add_u8(&msg, angle0);
    crumbs_msg_add_u8(&msg, angle1);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Sweep a servo from start to end angle.
 *
 * The peripheral will animate the servo from start to end angle,
 * pausing step_ms milliseconds between each degree.
 *
 * @param ctx         CRUMBS controller context.
 * @param addr        I2C address of servo peripheral.
 * @param write_fn    I2C write function.
 * @param io          I2C context.
 * @param channel     Servo channel (0 or 1).
 * @param start_angle Starting angle (0-180).
 * @param end_angle   Ending angle (0-180).
 * @param step_ms     Delay per degree in milliseconds.
 * @return 0 on success, non-zero on error.
 */
static inline int servo_send_sweep(crumbs_context_t *ctx,
                                   uint8_t addr,
                                   crumbs_i2c_write_fn write_fn,
                                   void *io,
                                   uint8_t channel,
                                   uint8_t start_angle,
                                   uint8_t end_angle,
                                   uint8_t step_ms)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_SWEEP);
    crumbs_msg_add_u8(&msg, channel);
    crumbs_msg_add_u8(&msg, start_angle);
    crumbs_msg_add_u8(&msg, end_angle);
    crumbs_msg_add_u8(&msg, step_ms);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Center all servos to 90 degrees.
 *
 * @param ctx      CRUMBS controller context.
 * @param addr     I2C address of servo peripheral.
 * @param write_fn I2C write function.
 * @param io       I2C context.
 * @return 0 on success, non-zero on error.
 */
static inline int servo_send_center_all(crumbs_context_t *ctx,
                                        uint8_t addr,
                                        crumbs_i2c_write_fn write_fn,
                                        void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_CENTER_ALL);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

/**
 * @brief Request current servo angles (peripheral responds on next I2C read).
 *
 * @param ctx      CRUMBS controller context.
 * @param addr     I2C address of servo peripheral.
 * @param write_fn I2C write function.
 * @param io       I2C context.
 * @return 0 on success, non-zero on error.
 */
static inline int servo_send_get_angles(crumbs_context_t *ctx,
                                        uint8_t addr,
                                        crumbs_i2c_write_fn write_fn,
                                        void *io)
{
    crumbs_message_t msg;
    crumbs_msg_init(&msg, SERVO_TYPE_ID, SERVO_CMD_GET_ANGLES);
    return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
}

#ifdef __cplusplus
}
#endif

#endif /* SERVO_COMMANDS_H */
