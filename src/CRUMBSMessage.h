#ifndef CRUMBSMESSAGE_H
#define CRUMBSMESSAGE_H

#include <stdint.h>

// Define the fixed message size
#define CRUMBS_MESSAGE_SIZE 28 // 1 + 1 + 1 + 24 + 1 = 28 bytes

struct CRUMBSMessage {
    uint8_t sliceID;         // Unique ID for the slice, 1 byte
    uint8_t typeID;          // Type identifier of the slice, 1 byte
    uint8_t commandType;     // Command or action identifier, 1 byte
    float data[6];           // Data payload, 6 floats = 24 bytes
    uint8_t errorFlags;      // Error or status flags, 1 byte
};

#endif // CRUMBSMESSAGE_H
