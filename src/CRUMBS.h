// File: lib/CRUMBS/CRUMBS.h

#ifndef CRUMBS_H
#define CRUMBS_H

#include <Arduino.h>       // Include the Arduino standard library
#include <Wire.h>          // Include the Wire library for I2C communication
#include "CRUMBSMessage.h" // Include our CRUMBSMessage class

#define TWI_CLOCK_FREQ 100000 /**< I2C clock frequency in Hz, 100000 is standard */

/**
 * @def CRUMBS_DEBUG
 * @brief Uncomment to enable debugging messages.
 */

//#define CRUMBS_DEBUG

#ifdef CRUMBS_DEBUG

#define CRUMBS_DEBUG_PRINT(...)  \
    Serial.print(F("CRUMBS: ")); \
    Serial.print(__VA_ARGS__)

#define CRUMBS_DEBUG_PRINTLN(...) \
    Serial.print(F("CRUMBS: "));  \
    Serial.println(__VA_ARGS__)

#else

#define CRUMBS_DEBUG_PRINT(...)
#define CRUMBS_DEBUG_PRINTLN(...)

#endif

/**
 * @class CRUMBS
 * @brief Handles I2C communication for CRUMBS protocol.
 */
class CRUMBS
{
public:
    /**
     * @brief Constructs a CRUMBS instance.
     *
     * @param isController Set to true if this instance is the I2C controller.
     * @param address I2C address for peripheral mode. Ignored if isController is true.
     */
    CRUMBS(bool isController = false, uint8_t address = 0);

    /**
     * @brief Initializes the I2C communication and registers event handlers.
     */
    void begin();

    /**
     * @brief Sends a CRUMBSMessage to the specified I2C target address.
     *
     * @param message The CRUMBSMessage to send.
     * @param targetAddress The I2C address of the target device.
     */
    void sendMessage(const CRUMBSMessage &message, uint8_t targetAddress);

    /**
     * @brief Receives a CRUMBSMessage from the I2C bus.
     *
     * @param message Reference to store the received message.
     */
    void receiveMessage(CRUMBSMessage &message);

    /**
     * @brief Registers a callback function to handle received messages.
     *
     * @param callback Function pointer taking a CRUMBSMessage reference.
     */
    void onReceive(void (*callback)(CRUMBSMessage &));

    /**
     * @brief Registers a callback function to handle request events.
     *
     * @param callback Function pointer with no parameters.
     */
    void onRequest(void (*callback)());

    /**
     * @brief Retrieves the I2C address of this instance.
     *
     * @return uint8_t The I2C address.
     */
    uint8_t getAddress() const;

    /**
     * @brief Encodes a CRUMBSMessage into a byte buffer.
     *
     * @param message The message to encode.
     * @param buffer The buffer to store the encoded message.
     * @param bufferSize Size of the buffer in bytes.
     * @return size_t Number of bytes written to the buffer.
     */
    size_t encodeMessage(const CRUMBSMessage &message, uint8_t *buffer, size_t bufferSize);

    /**
     * @brief Decodes a byte buffer into a CRUMBSMessage.
     *
     * @param buffer The buffer containing the encoded message.
     * @param bufferSize Size of the buffer in bytes.
     * @param message Reference to store the decoded message.
     * @return true If decoding was successful.
     * @return false If decoding failed due to insufficient buffer size.
     */
    bool decodeMessage(const uint8_t *buffer, size_t bufferSize, CRUMBSMessage &message);

private:
    uint8_t i2cAddress;                                 /**< I2C address of the device */
    bool controllerMode;                                    /**< Indicates if the instance is controller */
    void (*receiveCallback)(CRUMBSMessage &) = nullptr; /**< Callback for received messages */
    void (*requestCallback)() = nullptr;                /**< Callback for request events */

    /**
     * @brief Static I2C receive event handler.
     *
     * @param bytes Number of bytes received.
     */
    static void receiveEvent(int bytes);

    /**
     * @brief Static I2C request event handler.
     */
    static void requestEvent();

    static CRUMBS *instance; /**< Singleton instance for static callbacks */
};

#endif // CRUMBS_H
