#ifndef CRUMBSMESSAGE_H
#define CRUMBSMESSAGE_H

#include <stdint.h>

struct CRUMBSMessage {
    uint8_t sliceID;         // Unique ID for the slice
    uint8_t typeID;          // Type identifier of the slice
    uint8_t commandType;     // Command or action identifier
    float data[4];           // Data payload
    uint8_t errorFlags;      // Error or status flags
};

#endif // CRUMBSMESSAGE_H
