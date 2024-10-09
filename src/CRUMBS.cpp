#include "CRUMBS.h"

CRUMBS* CRUMBS::instance = nullptr;

CRUMBS::CRUMBS(bool isMaster, uint8_t address) : masterMode(isMaster) {
    if (masterMode) {
        i2cAddress = 0; // Master does not have an address
    } else {
        i2cAddress = address;
    }
    instance = this; // Assign singleton instance
    CRUMBS_DEBUG_PRINT(F("Initializing CRUMBS instance. Address: 0x"));
    CRUMBS_DEBUG_PRINTLN(i2cAddress, HEX);
}

void CRUMBS::begin() {
    CRUMBS_DEBUG_PRINTLN(F("Begin initialization..."));

    if (masterMode) {
        Wire.begin(); // Master mode
        Wire.setClock(100000); // Standard I2C clock speed
        CRUMBS_DEBUG_PRINTLN(F("Initialized as Master mode"));
    } else {
        Wire.begin(i2cAddress); // Slave mode with the specified address
        Wire.setClock(100000); // Standard I2C clock speed
        CRUMBS_DEBUG_PRINT(F("Initialized as Slave mode at address 0x"));
        CRUMBS_DEBUG_PRINTLN(i2cAddress, HEX);
    }

    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    CRUMBS_DEBUG_PRINTLN(F("I2C event handlers registered."));
}

size_t CRUMBS::encodeMessage(const CRUMBSMessage& message, uint8_t* buffer, size_t bufferSize) {
    const size_t MESSAGE_SIZE = CRUMBS_MESSAGE_SIZE; // 28 bytes
    if (bufferSize < MESSAGE_SIZE) {
        CRUMBS_DEBUG_PRINTLN(F("Buffer size too small for encoding message."));
        return 0;
    }

    size_t index = 0;

    // Serialize fields into buffer in order
    buffer[index++] = message.sliceID;
    buffer[index++] = message.typeID;
    buffer[index++] = message.commandType;

    // Serialize float data[6]
    for (int i = 0; i < 6; i++) {
        uint8_t* floatPtr = (uint8_t*)&message.data[i];
        for (int b = 0; b < sizeof(float); b++) {
            buffer[index++] = floatPtr[b];
        }
    }

    buffer[index++] = message.errorFlags;

    CRUMBS_DEBUG_PRINTLN(F("Message successfully encoded."));

    return index; // Should be 28
}

bool CRUMBS::decodeMessage(const uint8_t* buffer, size_t bufferSize, CRUMBSMessage& message) {
    const size_t MESSAGE_SIZE = CRUMBS_MESSAGE_SIZE; // 28 bytes
    if (bufferSize < MESSAGE_SIZE) {
        CRUMBS_DEBUG_PRINTLN(F("Buffer size too small for decoding message."));
        return false;
    }

    size_t index = 0;

    // Deserialize fields from buffer in order
    message.sliceID = buffer[index++];
    message.typeID = buffer[index++];
    message.commandType = buffer[index++];

    // Deserialize float data[6]
    for (int i = 0; i < 6; i++) {
        float value = 0.0;
        uint8_t* floatPtr = (uint8_t*)&value;
        for (int b = 0; b < sizeof(float); b++) {
            floatPtr[b] = buffer[index++];
        }
        message.data[i] = value;
    }

    message.errorFlags = buffer[index++];

    CRUMBS_DEBUG_PRINTLN(F("Message successfully decoded."));

    return true;
}

void CRUMBS::sendMessage(const CRUMBSMessage& message, uint8_t targetAddress) {
    CRUMBS_DEBUG_PRINTLN(F("Preparing to send message..."));

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t encodedSize = encodeMessage(message, buffer, sizeof(buffer));

    if (encodedSize == 0) {
        CRUMBS_DEBUG_PRINTLN(F("Failed to encode message. Aborting send."));
        return;
    }

    CRUMBS_DEBUG_PRINT(F("Encoded message size: "));
    CRUMBS_DEBUG_PRINTLN(encodedSize);

    Wire.beginTransmission(targetAddress);
    size_t bytesWritten = Wire.write(buffer, encodedSize);
    uint8_t error = Wire.endTransmission();

    CRUMBS_DEBUG_PRINT(F("Bytes written: "));
    CRUMBS_DEBUG_PRINTLN(bytesWritten);
    CRUMBS_DEBUG_PRINT(F("I2C Transmission Status: "));
    switch (error) {
        case 0: CRUMBS_DEBUG_PRINTLN(F("Success")); break;
        case 1: CRUMBS_DEBUG_PRINTLN(F("Data too long to fit in transmit buffer")); break;
        case 2: CRUMBS_DEBUG_PRINTLN(F("Received NACK on transmit of address (possible address mismatch)")); break;
        case 3: CRUMBS_DEBUG_PRINTLN(F("Received NACK on transmit of data")); break;
        case 4: CRUMBS_DEBUG_PRINTLN(F("Other error (e.g., bus error)")); break;
        default: CRUMBS_DEBUG_PRINTLN(F("Unknown error"));
    }

    CRUMBS_DEBUG_PRINTLN(F("Message transmission attempt complete."));
}

void CRUMBS::receiveMessage(CRUMBSMessage& message) {
    CRUMBS_DEBUG_PRINTLN(F("Receiving message..."));

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t index = 0;

    while (Wire.available() > 0 && index < sizeof(buffer)) {
        buffer[index++] = Wire.read();
    }

    CRUMBS_DEBUG_PRINT(F("Bytes received: "));
    CRUMBS_DEBUG_PRINTLN(index);

    if (index == 0) {
        CRUMBS_DEBUG_PRINTLN(F("No data received. Exiting receiveMessage."));
        return;
    }

    if (!decodeMessage(buffer, index, message)) {
        CRUMBS_DEBUG_PRINTLN(F("Failed to decode message."));
    } else {
        CRUMBS_DEBUG_PRINTLN(F("Message received and decoded successfully."));
    }
}

void CRUMBS::onReceive(void (*callback)(CRUMBSMessage&)) {
    receiveCallback = callback;
    CRUMBS_DEBUG_PRINTLN(F("onReceive callback function set."));
}

void CRUMBS::onRequest(void (*callback)()) {
    requestCallback = callback;
    CRUMBS_DEBUG_PRINTLN(F("onRequest callback function set."));
}

uint8_t CRUMBS::getAddress() const {
    return i2cAddress;
}

void CRUMBS::receiveEvent(int bytes) {
    CRUMBS_DEBUG_PRINT(F("Received event with "));
    CRUMBS_DEBUG_PRINT(bytes);
    CRUMBS_DEBUG_PRINTLN(F(" bytes."));

    if (bytes == 0) {
        CRUMBS_DEBUG_PRINTLN(F("No data received in event."));
        return;
    }

    if (instance && instance->receiveCallback) {
        CRUMBSMessage message;
        instance->receiveMessage(message);
        instance->receiveCallback(message);
    } else {
        CRUMBS_DEBUG_PRINTLN(F("receiveCallback function is not set."));
    }
}

void CRUMBS::requestEvent() {
    CRUMBS_DEBUG_PRINTLN(F("Request event triggered."));

    if (instance && instance->requestCallback) {
        instance->requestCallback();
    } else {
        CRUMBS_DEBUG_PRINTLN(F("requestCallback function is not set."));
    }
}
