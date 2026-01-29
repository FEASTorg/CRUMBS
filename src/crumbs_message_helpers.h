/**
 * @file crumbs_message_helpers.h
 * @brief Message building and payload reading helpers for CRUMBS.
 *
 * This header provides inline helpers for:
 * - Building messages with type-safe payload construction
 * - Reading payload fields with bounds checking
 *
 * All functions are static inline for zero overhead.
 * No static/global state — pure functions only.
 *
 * @code
 * // Building a message (controller side)
 * crumbs_message_t msg;
 * crumbs_msg_init(&msg, MY_TYPE_ID, MY_CMD);
 * crumbs_msg_add_u8(&msg, index);
 * crumbs_msg_add_u16(&msg, value);
 * crumbs_controller_send(&ctx, addr, &msg, write_fn, io);
 *
 * // Reading payload (peripheral handler)
 * void my_handler(crumbs_context_t *ctx, uint8_t cmd,
 *                 const uint8_t *data, uint8_t len, void *user) {
 *     uint8_t index;
 *     uint16_t value;
 *     if (crumbs_msg_read_u8(data, len, 0, &index) == 0 &&
 *         crumbs_msg_read_u16(data, len, 1, &value) == 0) {
 *         // use index, value
 *     }
 * }
 * @endcode
 */

#ifndef CRUMBS_MESSAGE_HELPERS_H
#define CRUMBS_MESSAGE_HELPERS_H

#include "crumbs_message.h"
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* ============================================================================
     * Message Building
     * ============================================================================ */

    /**
     * @brief Initialize a message with type_id and opcode.
     *
     * Clears all fields and sets the header. data_len starts at 0.
     *
     * @param msg         Pointer to message to initialize.
     * @param type_id     Device/module type identifier.
     * @param opcode Command opcode.
     */
    static inline void crumbs_msg_init(crumbs_message_t *msg,
                                       uint8_t type_id,
                                       uint8_t opcode)
    {
        memset(msg, 0, sizeof(*msg));
        msg->type_id = type_id;
        msg->opcode = opcode;
    }

    /**
     * @brief Add a uint8_t to the message payload.
     *
     * @param msg Pointer to message.
     * @param val Value to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_u8(crumbs_message_t *msg, uint8_t val)
    {
        if (msg->data_len >= CRUMBS_MAX_PAYLOAD)
            return -1;
        msg->data[msg->data_len++] = val;
        return 0;
    }

    /**
     * @brief Add a uint16_t (little-endian) to the message payload.
     *
     * @param msg Pointer to message.
     * @param val Value to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_u16(crumbs_message_t *msg, uint16_t val)
    {
        if ((size_t)msg->data_len + 2 > CRUMBS_MAX_PAYLOAD)
            return -1;
        msg->data[msg->data_len++] = (uint8_t)(val & 0xFF);
        msg->data[msg->data_len++] = (uint8_t)(val >> 8);
        return 0;
    }

    /**
     * @brief Add a uint32_t (little-endian) to the message payload.
     *
     * @param msg Pointer to message.
     * @param val Value to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_u32(crumbs_message_t *msg, uint32_t val)
    {
        if ((size_t)msg->data_len + 4 > CRUMBS_MAX_PAYLOAD)
            return -1;
        msg->data[msg->data_len++] = (uint8_t)(val);
        msg->data[msg->data_len++] = (uint8_t)(val >> 8);
        msg->data[msg->data_len++] = (uint8_t)(val >> 16);
        msg->data[msg->data_len++] = (uint8_t)(val >> 24);
        return 0;
    }

    /**
     * @brief Add an int8_t to the message payload.
     *
     * @param msg Pointer to message.
     * @param val Value to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_i8(crumbs_message_t *msg, int8_t val)
    {
        return crumbs_msg_add_u8(msg, (uint8_t)val);
    }

    /**
     * @brief Add an int16_t (little-endian) to the message payload.
     *
     * @param msg Pointer to message.
     * @param val Value to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_i16(crumbs_message_t *msg, int16_t val)
    {
        return crumbs_msg_add_u16(msg, (uint16_t)val);
    }

    /**
     * @brief Add an int32_t (little-endian) to the message payload.
     *
     * @param msg Pointer to message.
     * @param val Value to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_i32(crumbs_message_t *msg, int32_t val)
    {
        return crumbs_msg_add_u32(msg, (uint32_t)val);
    }

    /**
     * @brief Add a float to the message payload.
     *
     * @warning Uses memcpy with native byte order. Portable between identical
     *          architectures only. For cross-architecture communication,
     *          serialize the float manually to a fixed format.
     *
     * @param msg Pointer to message.
     * @param val Value to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_float(crumbs_message_t *msg, float val)
    {
        if ((size_t)msg->data_len + sizeof(float) > CRUMBS_MAX_PAYLOAD)
            return -1;
        memcpy(&msg->data[msg->data_len], &val, sizeof(float));
        msg->data_len += sizeof(float);
        return 0;
    }

    /**
     * @brief Add raw bytes to the message payload.
     *
     * @param msg  Pointer to message.
     * @param data Pointer to bytes to append.
     * @param len  Number of bytes to append.
     * @return 0 on success, -1 if payload would overflow.
     */
    static inline int crumbs_msg_add_bytes(crumbs_message_t *msg,
                                           const void *data, uint8_t len)
    {
        if ((size_t)msg->data_len + len > CRUMBS_MAX_PAYLOAD)
            return -1;
        memcpy(&msg->data[msg->data_len], data, len);
        msg->data_len += len;
        return 0;
    }

    /* ============================================================================
     * Payload Reading
     * ============================================================================ */

    /**
     * @brief Read a uint8_t from payload at offset.
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to store result.
     * @return 0 on success, -1 if offset out of bounds.
     */
    static inline int crumbs_msg_read_u8(const uint8_t *data, uint8_t len,
                                         uint8_t offset, uint8_t *out)
    {
        if (offset >= len)
            return -1;
        *out = data[offset];
        return 0;
    }

    /**
     * @brief Read a uint16_t (little-endian) from payload at offset.
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to store result.
     * @return 0 on success, -1 if offset out of bounds.
     */
    static inline int crumbs_msg_read_u16(const uint8_t *data, uint8_t len,
                                          uint8_t offset, uint16_t *out)
    {
        if (offset + 2 > len)
            return -1;
        *out = (uint16_t)data[offset] | ((uint16_t)data[offset + 1] << 8);
        return 0;
    }

    /**
     * @brief Read a uint32_t (little-endian) from payload at offset.
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to store result.
     * @return 0 on success, -1 if offset out of bounds.
     */
    static inline int crumbs_msg_read_u32(const uint8_t *data, uint8_t len,
                                          uint8_t offset, uint32_t *out)
    {
        if (offset + 4 > len)
            return -1;
        *out = (uint32_t)data[offset] |
               ((uint32_t)data[offset + 1] << 8) |
               ((uint32_t)data[offset + 2] << 16) |
               ((uint32_t)data[offset + 3] << 24);
        return 0;
    }

    /**
     * @brief Read an int8_t from payload at offset.
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to store result.
     * @return 0 on success, -1 if offset out of bounds.
     */
    static inline int crumbs_msg_read_i8(const uint8_t *data, uint8_t len,
                                         uint8_t offset, int8_t *out)
    {
        uint8_t tmp;
        if (crumbs_msg_read_u8(data, len, offset, &tmp) != 0)
            return -1;
        *out = (int8_t)tmp;
        return 0;
    }

    /**
     * @brief Read an int16_t (little-endian) from payload at offset.
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to store result.
     * @return 0 on success, -1 if offset out of bounds.
     */
    static inline int crumbs_msg_read_i16(const uint8_t *data, uint8_t len,
                                          uint8_t offset, int16_t *out)
    {
        uint16_t tmp;
        if (crumbs_msg_read_u16(data, len, offset, &tmp) != 0)
            return -1;
        *out = (int16_t)tmp;
        return 0;
    }

    /**
     * @brief Read an int32_t (little-endian) from payload at offset.
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to store result.
     * @return 0 on success, -1 if offset out of bounds.
     */
    static inline int crumbs_msg_read_i32(const uint8_t *data, uint8_t len,
                                          uint8_t offset, int32_t *out)
    {
        uint32_t tmp;
        if (crumbs_msg_read_u32(data, len, offset, &tmp) != 0)
            return -1;
        *out = (int32_t)tmp;
        return 0;
    }

    /**
     * @brief Read a float from payload at offset.
     *
     * @warning Uses memcpy with native byte order. See crumbs_msg_add_float().
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to store result.
     * @return 0 on success, -1 if offset out of bounds.
     */
    static inline int crumbs_msg_read_float(const uint8_t *data, uint8_t len,
                                            uint8_t offset, float *out)
    {
        if (offset + sizeof(float) > len)
            return -1;
        memcpy(out, &data[offset], sizeof(float));
        return 0;
    }

    /**
     * @brief Read raw bytes from payload at offset.
     *
     * @param data   Pointer to payload bytes.
     * @param len    Length of payload.
     * @param offset Byte offset to read from.
     * @param out    Pointer to buffer to store result.
     * @param count  Number of bytes to read.
     * @return 0 on success, -1 if offset+count out of bounds.
     */
    static inline int crumbs_msg_read_bytes(const uint8_t *data, uint8_t len,
                                            uint8_t offset, void *out, uint8_t count)
    {
        if (offset + count > len)
            return -1;
        memcpy(out, &data[offset], count);
        return 0;
    }

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_MESSAGE_HELPERS_H */
