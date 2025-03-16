// File: src/CRUMBS.cpp

#include "CRUMBS.h"

// Initialize the static singleton instance to nullptr
CRUMBS *CRUMBS::instance = nullptr;

/**
 * @brief Constructs a CRUMBS instance and sets the singleton instance.
 *
 * @param isController Set to true if this instance is the I2C controller.
 * @param address I2C address for peripheral mode. Ignored if isController is true.
 */
CRUMBS::CRUMBS(bool isController, uint8_t address) : controllerMode(isController)
{
    if (controllerMode)
    {
        i2cAddress = 0; // Controller does not have an address
    }
    else
    {
        i2cAddress = address;
    }
    instance = this; // Assign singleton instance
    CRUMBS_DEBUG_PRINT(F("Initializing CRUMBS instance. Address: 0x"));
    CRUMBS_DEBUG_PRINTLN(i2cAddress, HEX);
}

/**
 * @brief Initializes the I2C communication and registers event handlers.
 */
void CRUMBS::begin()
{
    CRUMBS_DEBUG_PRINTLN(F("Begin initialization..."));

    if (controllerMode)
    {
        Wire.begin();                  // Initialize as I2C controller
        Wire.setClock(TWI_CLOCK_FREQ); // Set I2C clock speed
        CRUMBS_DEBUG_PRINTLN(F("Initialized as Controller mode"));
    }
    else
    {
        Wire.begin(i2cAddress);        // Initialize as I2C peripheral with specified address
        Wire.setClock(TWI_CLOCK_FREQ); // Set I2C clock speed
        CRUMBS_DEBUG_PRINT(F("Initialized as Peripheral mode at address 0x"));
        CRUMBS_DEBUG_PRINTLN(i2cAddress, HEX);
    }

    // Register I2C event handlers
    Wire.onReceive(receiveEvent);
    Wire.onRequest(requestEvent);
    CRUMBS_DEBUG_PRINTLN(F("I2C event handlers registered."));
}

/**
 * @brief Encodes a CRUMBSMessage into a byte buffer.
 *
 * @param message The message to encode.
 * @param buffer The buffer to store the encoded message.
 * @param bufferSize Size of the buffer in bytes.
 * @return size_t Number of bytes written to the buffer.
 */
size_t CRUMBS::encodeMessage(const CRUMBSMessage &message, uint8_t *buffer, size_t bufferSize)
{
    if (bufferSize < CRUMBS_MESSAGE_SIZE)
    {
        CRUMBS_DEBUG_PRINTLN(F("Buffer size too small for encoding message."));
        return 0;
    }

    size_t index = 0;

    // Serialize fields into buffer in order
    buffer[index++] = message.typeID;
    buffer[index++] = message.commandType;

    // Serialize float data[6]
    for (int i = 0; i < 6; i++)
    {
        uint8_t *floatPtr = (uint8_t *)&message.data[i];
        for (int b = 0; b < sizeof(float); b++)
        {
            buffer[index++] = floatPtr[b];
        }
    }

    buffer[index++] = message.errorFlags;

    CRUMBS_DEBUG_PRINTLN(F("Message successfully encoded."));

    return index; // Should be 28
}

/**
 * @brief Decodes a byte buffer into a CRUMBSMessage.
 *
 * @param buffer The buffer containing the encoded message.
 * @param bufferSize Size of the buffer in bytes.
 * @param message Reference to store the decoded message.
 * @return true If decoding was successful.
 * @return false If decoding failed due to insufficient buffer size.
 */
bool CRUMBS::decodeMessage(const uint8_t *buffer, size_t bufferSize, CRUMBSMessage &message)
{
    if (bufferSize < CRUMBS_MESSAGE_SIZE)
    {
        CRUMBS_DEBUG_PRINTLN(F("Buffer size too small for decoding message."));
        return false;
    }

    size_t index = 0;

    // Deserialize fields from buffer in order
    message.typeID = buffer[index++];
    message.commandType = buffer[index++];

    // Deserialize float data[6]
    for (int i = 0; i < 6; i++)
    {
        float value = 0.0;
        uint8_t *floatPtr = (uint8_t *)&value;
        for (int b = 0; b < sizeof(float); b++)
        {
            floatPtr[b] = buffer[index++];
        }
        message.data[i] = value;
    }

    message.errorFlags = buffer[index++];

    CRUMBS_DEBUG_PRINTLN(F("Message successfully decoded."));

    return true;
}

/**
 * @brief Sends a CRUMBSMessage to the specified I2C target address.
 *
 * @param message The message to send.
 * @param targetAddress The I2C address of the target device.
 */
void CRUMBS::sendMessage(const CRUMBSMessage &message, uint8_t targetAddress)
{
    CRUMBS_DEBUG_PRINTLN(F("Preparing to send message..."));

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t encodedSize = encodeMessage(message, buffer, sizeof(buffer));

    if (encodedSize == 0)
    {
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
    switch (error)
    {
    case 0:
        CRUMBS_DEBUG_PRINTLN(F("Success"));
        break;
    case 1:
        CRUMBS_DEBUG_PRINTLN(F("Data too long to fit in transmit buffer"));
        break;
    case 2:
        CRUMBS_DEBUG_PRINTLN(F("Received NACK on transmit of address (possible address mismatch)"));
        break;
    case 3:
        CRUMBS_DEBUG_PRINTLN(F("Received NACK on transmit of data"));
        break;
    case 4:
        CRUMBS_DEBUG_PRINTLN(F("Other error (e.g., bus error)"));
        break;
    default:
        CRUMBS_DEBUG_PRINTLN(F("Unknown error"));
    }

    CRUMBS_DEBUG_PRINTLN(F("Message transmission attempt complete."));
}

/**
 * @brief Receives a CRUMBSMessage from the I2C bus.
 *
 * @param message Reference to store the received message.
 */
void CRUMBS::receiveMessage(CRUMBSMessage &message)
{
    CRUMBS_DEBUG_PRINTLN(F("Receiving message..."));

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t index = 0;

    // Read available bytes into buffer
    while (Wire.available() > 0 && index < sizeof(buffer))
    {
        buffer[index++] = Wire.read();
    }

    CRUMBS_DEBUG_PRINT(F("Bytes received in buffer: "));
    CRUMBS_DEBUG_PRINTLN(index);

    if (index == 0)
    {
        CRUMBS_DEBUG_PRINTLN(F("No data received. Exiting receiveMessage."));
        return;
    }

    // Decode the received message
    if (!decodeMessage(buffer, index, message))
    {
        CRUMBS_DEBUG_PRINTLN(F("Failed to decode message."));
    }
    else
    {
        CRUMBS_DEBUG_PRINTLN(F("Message received and decoded successfully."));
    }
}

/**
 * @brief Registers a callback function to handle received messages.
 *
 * @param callback Function pointer taking a CRUMBSMessage reference.
 */
void CRUMBS::onReceive(void (*callback)(CRUMBSMessage &))
{
    receiveCallback = callback;
    CRUMBS_DEBUG_PRINTLN(F("onReceive callback function set."));
}

/**
 * @brief Registers a callback function to handle request events.
 *
 * @param callback Function pointer with no parameters.
 */
void CRUMBS::onRequest(void (*callback)())
{
    requestCallback = callback;
    CRUMBS_DEBUG_PRINTLN(F("onRequest callback function set."));
}

/**
 * @brief Retrieves the I2C address of this instance.
 *
 * @return uint8_t The I2C address.
 */
uint8_t CRUMBS::getAddress() const
{
    return i2cAddress;
}

/**
 * @brief Static I2C receive event handler.
 *
 * @param bytes Number of bytes received.
 */
void CRUMBS::receiveEvent(int bytes)
{
    CRUMBS_DEBUG_PRINT(F("Received event with num bytes: "));
    CRUMBS_DEBUG_PRINTLN(bytes);

    if (bytes == 0)
    {
        CRUMBS_DEBUG_PRINTLN(F("No data received in event."));
        return;
    }

    // If a receive callback is set, process the received message
    if (instance && instance->receiveCallback)
    {
        CRUMBSMessage message;
        instance->receiveMessage(message);
        instance->receiveCallback(message);
    }
    else
    {
        CRUMBS_DEBUG_PRINTLN(F("receiveCallback function is not set."));
    }
}

/**
 * @brief Static I2C request event handler.
 */
void CRUMBS::requestEvent()
{
    CRUMBS_DEBUG_PRINTLN(F("Request event triggered."));

    // If a request callback is set, execute it to send data
    if (instance && instance->requestCallback)
    {
        instance->requestCallback();
    }
    else
    {
        CRUMBS_DEBUG_PRINTLN(F("requestCallback function is not set."));
    }
}
