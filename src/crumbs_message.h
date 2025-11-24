#ifndef CRUMBS_MESSAGE_H
#define CRUMBS_MESSAGE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Number of float payload elements in a CRUMBS message.
 *
 * The wire format is:
 *   - type_id      : 1 byte
 *   - command_type : 1 byte
 *   - data[7]      : 7 * 4 bytes (float32, little-endian)
 *   - crc8         : 1 byte
 *
 * Total serialized payload length = 31 bytes.
 */
#define CRUMBS_DATA_LENGTH 7u
#define CRUMBS_MESSAGE_SIZE 31u

    /**
     * @brief Fixed-size message structure for CRUMBS communication.
     *
     * Note: slice_address is *not* serialized by the core encoder/decoder;
     * it is a logical address your application can use for routing.
     */
    typedef struct crumbs_message_s
    {
        uint8_t slice_address;          /**< Logical slice address (not serialized) */
        uint8_t type_id;                /**< Identifier for the module type */
        uint8_t command_type;           /**< Command or opcode identifier */
        float data[CRUMBS_DATA_LENGTH]; /**< Payload data fields (7 floats) */
        uint8_t crc8;                   /**< CRC-8 over serialized payload */
    } crumbs_message_t;

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_MESSAGE_H */
