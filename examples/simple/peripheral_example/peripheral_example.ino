/**
 * @file crumbs_slice_test.ino
 * @brief CRUMBS Slice example sketch to receive messages from a CRUMBS Controller and respond to requests.
 */

#define CRUMBS_DEBUG

#include <crumbs/crumbs.h>
#include <crumbs/crumbs_arduino.h>

/**
 * @brief I2C address for this Slice device.
 *
 * @note Ensure this matches the address specified by the Controller when sending messages.
 */
#define SLICE_I2C_ADDRESS 0x09 // Example I2C address

static crumbs_context_t crumbsSlice;

namespace
{
    uint32_t lastCrcReport = 0;
    bool lastCrcValid = true;

    void reportCrcStatus(const __FlashStringHelper *context)
    {
        const uint32_t current = crumbs_get_crc_errors(&crumbsSlice);
        const bool lastValid = crumbs_last_crc_valid(&crumbsSlice);

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
 * @brief Callback function to handle received crumbs_message_ts from the Controller.
 *
 * @param message The received crumbs_message_t.
 */
void handleMessage(crumbs_message_t &message)
{
    Serial.println(F("Slice: Received Message:"));
    Serial.print(F("type_id: "));
    Serial.println(message.type_id);
    Serial.print(F("command_type: "));
    Serial.println(message.command_type);
    Serial.print(F("data: "));
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        Serial.print(message.data[i]);
        Serial.print(F(" "));
    }
    Serial.println();
    Serial.print(F("crc8: 0x"));
    Serial.println(message.crc8, HEX);

    // Process the message based on command_type
    switch (message.command_type)
    {
    case 0:
        // command_type 0: Data Format Request
        Serial.println(F("Slice: Data Format Request Received."));
        // Perform actions to alter the data mapped to the callback message to be sent on wire request
        break;

    case 1:
        // command_type 1: Example Command (e.g., Set Parameters)
        Serial.println(F("Slice: Set Parameters Command Received."));
        // Example: Update internal state based on data
        break;

        // Add more case blocks for different command_types as needed

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
 * @note This function sends a crumbs_message_t back to the Controller in response to a request.
 */
void handleRequest()
{
    Serial.println(F("Slice: Controller requested data, sending response..."));

    // Prepare response message
    crumbs_message_t responseMessage = {};
    responseMessage.type_id = 1;      /**< SLICE type ID */
    responseMessage.command_type = 0; /**< command_type 0 for status response */

    // Populate data fields with example data
    responseMessage.data[0] = 42.0f; /**< Example data0 */
    responseMessage.data[1] = 1.0f;  /**< Example data1 */
    responseMessage.data[2] = 2.0f;  /**< Example data2 */
    responseMessage.data[3] = 3.0f;  /**< Example data3 */
    responseMessage.data[4] = 4.0f;  /**< Example data4 */
    responseMessage.data[5] = 5.0f;  /**< Example data5 */
    responseMessage.data[6] = 6.0f;  /**< Example data6 */

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t encodedSize = crumbs_encode_message(&responseMessage, buffer, sizeof(buffer));

    if (encodedSize == 0)
    {
        Serial.println(F("Slice: Failed to encode response message."));
        reportCrcStatus(F("encode"));
        return;
    }

    // Send the encoded message back to the Controller
    Wire.write(buffer, encodedSize);
    Serial.println(F("Slice: Response message sent."));
    reportCrcStatus(F("encode"));
}

/**
 * @brief Sets up the Slice device, initializes serial communication, registers callbacks, and prepares for communication.
 */
void setup()
{
    Serial.begin(115200); /**< Initialize serial communication */

    crumbs_arduino_init_peripheral(&crumbsSlice, SLICE_I2C_ADDRESS);

    crumbs_reset_crc_errors(&crumbsSlice);
    Serial.println(F("Slice: CRC diagnostics reset."));
    reportCrcStatus(F("startup"));

    crumbs_set_callbacks(&crumbsSlice, handleMessage, handleRequest, nullptr);

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
