/**
 * @file mock_ops.h
 * @brief Mock device command definitions for handler pattern demonstration
 *
 * This file demonstrates the pattern for defining device commands.
 * Copy and modify this template for your own device types.
 *
 * This header defines a mock device (Type ID 0x10) for demonstrating
 * the CRUMBS handler pattern without hardware complexity.
 *
 * Commands:
 * - MOCK_OP_ECHO:         Echo data back (store for later retrieval)
 * - MOCK_OP_SET_HEARTBEAT: Set LED heartbeat period
 * - MOCK_OP_TOGGLE:       Toggle heartbeat enable/disable
 * - MOCK_OP_GET_ECHO:     Request stored echo data (via SET_REPLY)
 * - MOCK_OP_GET_STATUS:   Request state and heartbeat period (via SET_REPLY)
 * - MOCK_OP_GET_INFO:     Request device info (via SET_REPLY)
 */

#ifndef MOCK_OPS_H
#define MOCK_OPS_H

#include "crumbs.h"
#include "crumbs_message_helpers.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Device Identity
 * ============================================================================ */

/** @brief Type ID for mock demonstration device. */
#define MOCK_TYPE_ID 0x10

/* ============================================================================
 * Command Definitions
 * ============================================================================ */

/**
 * @brief Echo operation - stores data for later retrieval.
 * Payload (SET): [data bytes...]
 * Reply (GET via SET_REPLY): [echoed data bytes...]
 */
#define MOCK_OP_ECHO 0x01

/**
 * @brief Set heartbeat operation - sets LED heartbeat period.
 * Payload: [period_ms:u16] (little-endian, milliseconds between pulses)
 */
#define MOCK_OP_SET_HEARTBEAT 0x02

/**
 * @brief Toggle operation - enables/disables heartbeat.
 * Payload: none
 */
#define MOCK_OP_TOGGLE 0x03

/**
 * @brief Request stored echo data.
 * Payload: none (reply contains previously stored echo data)
 */
#define MOCK_OP_GET_ECHO 0x80

/**
 * @brief Request current status (state + heartbeat period).
 * Payload: none (reply contains [state:u8, period_ms:u16])
 */
#define MOCK_OP_GET_STATUS 0x81

/**
 * @brief Request device info (version string).
 * Payload: none (reply contains device info string)
 * Note: Also returned as default (opcode 0) reply.
 */
#define MOCK_OP_GET_INFO 0x82

    /* ============================================================================
     * Controller Side: Command Senders
     *
     * These functions build and send commands to a mock peripheral.
     * ============================================================================ */

    /**
     * @brief Send echo data to peripheral (stores for later retrieval).
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of mock peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context (Wire*, linux handle, etc.).
     * @param data     Data bytes to echo.
     * @param len      Number of data bytes (0-27).
     * @return 0 on success, non-zero on error.
     */
    static inline int mock_send_echo(crumbs_context_t *ctx,
                                     uint8_t addr,
                                     crumbs_i2c_write_fn write_fn,
                                     void *io,
                                     const uint8_t *data,
                                     uint8_t len)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, MOCK_TYPE_ID, MOCK_OP_ECHO);
        for (uint8_t i = 0; i < len; i++)
        {
            crumbs_msg_add_u8(&msg, data[i]);
        }
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Set LED heartbeat period.
     *
     * @param ctx       CRUMBS controller context.
     * @param addr      I2C address of mock peripheral.
     * @param write_fn  I2C write function.
     * @param io        I2C context.
     * @param period_ms Heartbeat period in milliseconds (0 = off, 1-65535 = pulse period).
     * @return 0 on success, non-zero on error.
     */
    static inline int mock_send_heartbeat(crumbs_context_t *ctx,
                                          uint8_t addr,
                                          crumbs_i2c_write_fn write_fn,
                                          void *io,
                                          uint16_t period_ms)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, MOCK_TYPE_ID, MOCK_OP_SET_HEARTBEAT);
        crumbs_msg_add_u16(&msg, period_ms);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Send toggle command to peripheral (flips state bit).
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of mock peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int mock_send_toggle(crumbs_context_t *ctx,
                                       uint8_t addr,
                                       crumbs_i2c_write_fn write_fn,
                                       void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, MOCK_TYPE_ID, MOCK_OP_TOGGLE);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Query stored echo data (peripheral will respond on next I2C read).
     *
     * Uses SET_REPLY pattern (0xFE) to request echo buffer contents.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of mock peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int mock_query_echo(crumbs_context_t *ctx,
                                      uint8_t addr,
                                      crumbs_i2c_write_fn write_fn,
                                      void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, MOCK_TYPE_ID, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&msg, MOCK_OP_GET_ECHO);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Query current status (state + heartbeat period).
     *
     * Uses SET_REPLY pattern (0xFE) to request status.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of mock peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int mock_query_status(crumbs_context_t *ctx,
                                        uint8_t addr,
                                        crumbs_i2c_write_fn write_fn,
                                        void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, MOCK_TYPE_ID, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&msg, MOCK_OP_GET_STATUS);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

    /**
     * @brief Query device info (peripheral will respond on next I2C read).
     *
     * Uses SET_REPLY pattern (0xFE) to request device information.
     *
     * @param ctx      CRUMBS controller context.
     * @param addr     I2C address of mock peripheral.
     * @param write_fn I2C write function.
     * @param io       I2C context.
     * @return 0 on success, non-zero on error.
     */
    static inline int mock_query_info(crumbs_context_t *ctx,
                                      uint8_t addr,
                                      crumbs_i2c_write_fn write_fn,
                                      void *io)
    {
        crumbs_message_t msg;
        crumbs_msg_init(&msg, MOCK_TYPE_ID, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&msg, MOCK_OP_GET_INFO);
        return crumbs_controller_send(ctx, addr, &msg, write_fn, io);
    }

#ifdef __cplusplus
}
#endif

#endif /* MOCK_OPS_H */
