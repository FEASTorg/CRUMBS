// File: lib/CRUMBS/CRUMBSMessage.h

#ifndef CRUMBSMESSAGE_H
#define CRUMBSMESSAGE_H

#include <stdint.h>

/**
 * @brief Fixed-size message structure for CRUMBS communication.
 */
struct CRUMBSMessage
{
    uint8_t sliceAddress; /**< Unique identifier for the target slice */
    uint8_t typeID;       /**< Identifier for the module type */
    uint8_t commandType;  /**< Command or action identifier */
    float data[6];        /**< Payload data (6 floating-point values) */
    uint8_t errorFlags;   /**< Error or status flags */
};

/**
 * @brief Size of the CRUMBSMessage structure in bytes.
 *
 * @note Calculated as: 1 + 1 + 24 (6 floats) + 1 = 27 bytes
 */

#define CRUMBS_MESSAGE_SIZE 27

#endif // CRUMBSMESSAGE_H