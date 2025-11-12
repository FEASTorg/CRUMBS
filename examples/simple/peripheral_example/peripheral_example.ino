/**
 * @file crumbs_slice_test.ino
 * @brief CRUMBS Slice example sketch to receive messages from a CRUMBS Controller and respond to requests.
 */

#define CRUMBS_DEBUG
#include <CRUMBS.h>

/**
 * @brief I2C address for this Slice device.
 *
 * @note Ensure this matches the address specified by the Controller when sending messages.
 */
#define SLICE_I2C_ADDRESS 0x09 // Example I2C address

/**
 * @brief Instantiate CRUMBS as a Slice (Peripheral).
 *
 * @note Pass 'false' to indicate Peripheral mode and provide the I2C address.
 */
CRUMBS crumbsSlice(false, SLICE_I2C_ADDRESS); // Peripheral mode, I2C address 0x08

namespace
{
    uint32_t lastCrcReport = 0;
    bool lastCrcValid = true;

    void reportCrcStatus(const __FlashStringHelper *context)
    {
        const uint32_t current = crumbsSlice.getCrcErrorCount();
        const bool lastValid = crumbsSlice.isLastCrcValid();

        if (current != lastCrcReport || lastValid != lastCrcValid)
        {
            Serial.print(F("Slice: CRC status ["));
            Serial.print(context);
            Serial.print(F("] errors="));
            Serial.print(current);
            Serial.print(F(" lastValid="));
            Serial.println(lastValid ? F("true") : F("false"));
            lastCrcReport = current;
            lastCrcValid = lastValid;
        }
    }
}

/**
 * @brief Callback function to handle received CRUMBSMessages from the Controller.
 *
 * @param message The received CRUMBSMessage.
 */
void handleMessage(CRUMBSMessage &message)
{
    Serial.println(F("Slice: Received Message:"));
    Serial.print(F("typeID: "));
    Serial.println(message.typeID);
    Serial.print(F("commandType: "));
    Serial.println(message.commandType);
    Serial.print(F("data: "));
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        Serial.print(message.data[i]);
        Serial.print(F(" "));
    }
    Serial.println();
    Serial.print(F("crc8: 0x"));
    Serial.println(message.crc8, HEX);

    // Process the message based on commandType
    switch (message.commandType)
    {
    case 0:
        // CommandType 0: Data Format Request
        Serial.println(F("Slice: Data Format Request Received."));
        // Perform actions to alter the data mapped to the callback message to be sent on wire request
        break;

    case 1:
        // CommandType 1: Example Command (e.g., Set Parameters)
        Serial.println(F("Slice: Set Parameters Command Received."));
        // Example: Update internal state based on data
        break;

        // Add more case blocks for different commandTypes as needed

    default:
        Serial.println(F("Slice: Unknown Command Type."));
        break;
    }

    Serial.println(F("Slice: Message processing complete."));
    reportCrcStatus(F("receive"));
}

/**
 * @brief Callback function to handle data requests from the Controller.
 *
 * @note This function sends a CRUMBSMessage back to the Controller in response to a request.
 */
void handleRequest()
{
    Serial.println(F("Slice: Controller requested data, sending response..."));

    // Prepare response message
    CRUMBSMessage responseMessage = {};
    responseMessage.typeID = 1;      /**< SLICE type ID */
    responseMessage.commandType = 0; /**< CommandType 0 for status response */

    // Populate data fields with example data
    responseMessage.data[0] = 42.0f; /**< Example data0 */
    responseMessage.data[1] = 1.0f;  /**< Example data1 */
    responseMessage.data[2] = 2.0f;  /**< Example data2 */
    responseMessage.data[3] = 3.0f;  /**< Example data3 */
    responseMessage.data[4] = 4.0f;  /**< Example data4 */
    responseMessage.data[5] = 5.0f;  /**< Example data5 */
    responseMessage.data[6] = 6.0f;  /**< Example data6 */

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t encodedSize = crumbsSlice.encodeMessage(responseMessage, buffer, sizeof(buffer));

    if (encodedSize == 0)
    {
        CRUMBS_DEBUG_PRINTLN(F("Slice: Failed to encode response message."));
        reportCrcStatus(F("encode"));
        return;
    }

    // Send the encoded message back to the Controller
    Wire.write(buffer, encodedSize);
    CRUMBS_DEBUG_PRINTLN(F("Slice: Response message sent."));
    reportCrcStatus(F("encode"));
}

/**
 * @brief Sets up the Slice device, initializes serial communication, registers callbacks, and prepares for communication.
 */
void setup()
{
    Serial.begin(115200); /**< Initialize serial communication */

    crumbsSlice.begin(); /**< Initialize CRUMBS communication */
    crumbsSlice.resetCrcErrorCount();
    Serial.println(F("Slice: CRC diagnostics reset."));
    reportCrcStatus(F("startup"));

    crumbsSlice.onRequest(handleRequest); // Register callback for data requests
    crumbsSlice.onReceive(handleMessage); // Register callback for received messages

    Serial.println(F("Slice ready and listening for messages..."));
}

/**
 *

 *
 * @brief Main loop that can perform periodic tasks if needed.
 *
 * @note Here, all actions are event-driven via callbacks.
 */
void loop()
{
    // No actions needed in loop for this example
    // All processing is handled via callbacks
    reportCrcStatus(F("loop"));
}
