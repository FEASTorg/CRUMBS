#ifndef CRUMBSMESSAGE_H
#define CRUMBSMESSAGE_H

#include <stdint.h>

/**
 * @brief Serialized size of a CRUMBS message in bytes.
 *
 * Calculated as: 1 (typeID) + 1 (commandType) + 28 (data[7]) + 1 (crc8) = 31 bytes
 */
#define CRUMBS_DATA_LENGTH 7
#define CRUMBS_MESSAGE_SIZE 31

/**
 * @brief Fixed-size message structure for CRUMBS communication.
 */
struct CRUMBSMessage
{
    uint8_t sliceAddress;           /**< Unique identifier for the target slice (not serialized) */
    uint8_t typeID;                 /**< Identifier for the module type */
    uint8_t commandType;            /**< Command or action identifier */
    float data[CRUMBS_DATA_LENGTH]; /**< Payload data (7 floating-point values) */
    uint8_t crc8;                   /**< CRC-8 checksum calculated over the serialized payload */
};

#endif // CRUMBSMESSAGE_H