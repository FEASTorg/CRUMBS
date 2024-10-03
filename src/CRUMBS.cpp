#include "CRUMBS.h"

CRUMBS* crumbsInstance = nullptr;

CRUMBS::CRUMBS(bool isMaster, uint8_t address) : masterMode(isMaster) {
    if (masterMode) {
        i2cAddress = 0; // Master does not have an address
    } else {
        i2cAddress = address;
    }
    crumbsInstance = this; // Singleton instance
    CRUMBS_DEBUG_PRINT("Initializing CRUMBS instance. Address: 0x");
    CRUMBS_DEBUG_PRINTLN(i2cAddress, HEX);
}

void CRUMBS::begin() {
    CRUMBS_DEBUG_PRINTLN("Begin initialization...");

    if (masterMode) {
        Wire.begin(); // Master mode
        Wire.setClock(100000); // Standard I2C clock speed
        CRUMBS_DEBUG_PRINTLN("Initialized as Master mode");
    } else {
        Wire.begin(i2cAddress); // Slave mode with the specified address
        Wire.setClock(100000); // Standard I2C clock speed
        CRUMBS_DEBUG_PRINT("Initialized as Slave mode at address 0x");
        CRUMBS_DEBUG_PRINTLN(i2cAddress, HEX);
    }

    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    CRUMBS_DEBUG_PRINTLN("onReceive and onRequest events registered.");
}

void CRUMBS::sendMessage(const CRUMBSMessage& message, uint8_t targetAddress) {
    CRUMBS_DEBUG_PRINTLN("Preparing to send message...");

    uint8_t buffer[64];
    size_t encodedSize = encodeMessage(message, buffer, sizeof(buffer));

    if (encodedSize == 0) {
        CRUMBS_DEBUG_PRINTLN("Failed to encode message. Aborting send.");
        return;
    }

    CRUMBS_DEBUG_PRINT("Encoded message size: ");
    CRUMBS_DEBUG_PRINTLN(encodedSize);

    Wire.beginTransmission(targetAddress);
    size_t bytesWritten = Wire.write(buffer, encodedSize);
    uint8_t error = Wire.endTransmission();

    CRUMBS_DEBUG_PRINT("Bytes written: ");
    CRUMBS_DEBUG_PRINTLN(bytesWritten);
    CRUMBS_DEBUG_PRINT("I2C Transmission Status: ");
    switch (error) {
        case 0: CRUMBS_DEBUG_PRINTLN("Success"); break;
        case 1: CRUMBS_DEBUG_PRINTLN("Data too long to fit in transmit buffer"); break;
        case 2: CRUMBS_DEBUG_PRINTLN("Received NACK on transmit of address (possible address mismatch)"); break;
        case 3: CRUMBS_DEBUG_PRINTLN("Received NACK on transmit of data"); break;
        case 4: CRUMBS_DEBUG_PRINTLN("Other error (e.g., bus error)"); break;
        default: CRUMBS_DEBUG_PRINTLN("Unknown error");
    }

    CRUMBS_DEBUG_PRINTLN("Message transmission attempt complete.");
}

void CRUMBS::receiveMessage(CRUMBSMessage& message) {
    CRUMBS_DEBUG_PRINTLN("Receiving message...");

    uint8_t buffer[64];
    size_t index = 0;

    while (Wire.available() > 0) {
        buffer[index++] = Wire.read();
    }

    CRUMBS_DEBUG_PRINT("Bytes received: ");
    CRUMBS_DEBUG_PRINTLN(index);

    if (index == 0) {
        CRUMBS_DEBUG_PRINTLN("No data received. Exiting receiveMessage.");
        return;
    }

    if (!decodeMessage(buffer, index, message)) {
        CRUMBS_DEBUG_PRINTLN("Failed to decode message.");
    } else {
        CRUMBS_DEBUG_PRINTLN("Message received and decoded successfully.");
    }
}

void CRUMBS::onReceive(void (*callback)(CRUMBSMessage&)) {
    receiveCallback = callback;
    CRUMBS_DEBUG_PRINTLN("onReceive callback function set.");
}

void CRUMBS::onRequest(void (*callback)()) {
    requestCallback = callback;
    CRUMBS_DEBUG_PRINTLN("onRequest callback function set.");
}

uint8_t CRUMBS::getAddress() const {
    return i2cAddress;
}

size_t CRUMBS::encodeMessage(const CRUMBSMessage& message, uint8_t* buffer, size_t bufferSize) {
    CborEncoder encoder, arrayEncoder;
    cbor_encoder_init(&encoder, buffer, bufferSize, 0);

    // Corrected array length to 8
    CborError err = cbor_encoder_create_array(&encoder, &arrayEncoder, 8);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error creating CBOR array");
        return 0;
    }

    err = cbor_encode_uint(&arrayEncoder, message.sliceID);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error encoding sliceID");
        return 0;
    }

    err = cbor_encode_uint(&arrayEncoder, message.typeID);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error encoding typeID");
        return 0;
    }

    err = cbor_encode_uint(&arrayEncoder, message.commandType);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error encoding commandType");
        return 0;
    }

    for (int i = 0; i < 4; i++) {
        err = cbor_encode_float(&arrayEncoder, message.data[i]);
        if (err != CborNoError) {
            CRUMBS_DEBUG_PRINT("Error encoding data element ");
            CRUMBS_DEBUG_PRINTLN(i);
            return 0;
        }
    }

    err = cbor_encode_uint(&arrayEncoder, message.errorFlags);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error encoding errorFlags");
        return 0;
    }

    err = cbor_encoder_close_container(&encoder, &arrayEncoder);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error closing CBOR container");
        return 0;
    }

    CRUMBS_DEBUG_PRINTLN("Message successfully encoded.");

    return cbor_encoder_get_buffer_size(&encoder, buffer);
}

bool CRUMBS::decodeMessage(const uint8_t* buffer, size_t bufferSize, CRUMBSMessage& message) {
    if (bufferSize == 0) {
        CRUMBS_DEBUG_PRINTLN("decodeMessage called with empty buffer.");
        return false;
    }

    CborParser parser;
    CborValue it, array;
    CborError err;

    err = cbor_parser_init(buffer, bufferSize, 0, &parser, &it);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error initializing CBOR parser");
        return false;
    }

    err = cbor_value_enter_container(&it, &array);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error entering CBOR container");
        return false;
    }

    uint64_t tempUint;
    float tempFloat;

    err = cbor_value_get_uint64(&array, &tempUint);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error reading sliceID.");
        return false;
    }
    message.sliceID = (uint8_t)tempUint;

    err = cbor_value_advance(&array);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error advancing to typeID.");
        return false;
    }

    err = cbor_value_get_uint64(&array, &tempUint);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error reading typeID.");
        return false;
    }
    message.typeID = (uint8_t)tempUint;

    err = cbor_value_advance(&array);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error advancing to commandType.");
        return false;
    }

    err = cbor_value_get_uint64(&array, &tempUint);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error reading commandType.");
        return false;
    }
    message.commandType = (uint8_t)tempUint;

    for (int i = 0; i < 4; i++) {
        err = cbor_value_advance(&array);
        if (err != CborNoError) {
            CRUMBS_DEBUG_PRINT("Error advancing to data element ");
            CRUMBS_DEBUG_PRINTLN(i);
            return false;
        }

        err = cbor_value_get_float(&array, &tempFloat);
        if (err != CborNoError) {
            CRUMBS_DEBUG_PRINT("Error reading data element ");
            CRUMBS_DEBUG_PRINTLN(i);
            return false;
        }
        message.data[i] = tempFloat;
    }

    err = cbor_value_advance(&array);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error advancing to errorFlags.");
        return false;
    }

    err = cbor_value_get_uint64(&array, &tempUint);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error reading errorFlags.");
        return false;
    }
    message.errorFlags = (uint8_t)tempUint;

    err = cbor_value_leave_container(&it, &array);
    if (err != CborNoError) {
        CRUMBS_DEBUG_PRINTLN("Error leaving CBOR container");
        return false;
    }

    CRUMBS_DEBUG_PRINTLN("Message successfully decoded.");

    return true;
}

void CRUMBS::receiveEvent(int bytes) {
    CRUMBS_DEBUG_PRINT("Received event with ");
    CRUMBS_DEBUG_PRINT(bytes);
    CRUMBS_DEBUG_PRINTLN(" bytes.");

    if (bytes == 0) {
        CRUMBS_DEBUG_PRINTLN("No data received in event.");
        return;
    }

    if (crumbsInstance && crumbsInstance->receiveCallback) {
        CRUMBSMessage message;
        crumbsInstance->receiveMessage(message);
        crumbsInstance->receiveCallback(message);
    } else {
        CRUMBS_DEBUG_PRINTLN("receiveCallback function is not set.");
    }
}

void CRUMBS::requestEvent() {
    CRUMBS_DEBUG_PRINTLN("Request event triggered.");

    if (crumbsInstance && crumbsInstance->requestCallback) {
        crumbsInstance->requestCallback();
    } else {
        CRUMBS_DEBUG_PRINTLN("requestCallback function is not set.");
    }
}
