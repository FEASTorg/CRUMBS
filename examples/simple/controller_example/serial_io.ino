#include <Arduino.h>
#include <Wire.h>
#include <CRUMBS.h>

extern CRUMBS crumbsController;

namespace
{
    uint32_t lastCrcReport = 0;
    bool lastCrcValid = true;

    void reportCrcStatus(const __FlashStringHelper *context)
    {
        const uint32_t current = crumbsController.getCrcErrorCount();
        const bool lastValid = crumbsController.isLastCrcValid();
        if (current != lastCrcReport || lastValid != lastCrcValid)
        {
            Serial.print(F("Controller: CRC status ["));
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
#include <CRUMBS.h>

/**
 * @brief Handles serial input from the user to send CRUMBSMessages to a specified target address.
 * @return none
 */
void handleSerialInput()
{
    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n'); // Read input until newline
        input.trim();                                // Remove any extra whitespace

        Serial.print(F("Controller: Received input from serial: "));
        Serial.println(input);

        // Check if the input starts with "request="
        if (input.startsWith("request="))
        {
            // Extract the address from the input string
            String addressString = input.substring(strlen("request="));
            addressString.trim();

            // Support hexadecimal format if it starts with "0x"
            uint8_t targetAddress = 0;
            if (addressString.startsWith("0x") || addressString.startsWith("0X"))
            {
                targetAddress = (uint8_t)strtol(addressString.c_str(), NULL, 16);
            }
            else
            {
                targetAddress = (uint8_t)addressString.toInt();
            }

            Serial.print(F("Controller: Requesting data from address: 0x"));
            Serial.println(targetAddress, HEX);

            // Request CRUMBS_MESSAGE_SIZE bytes from the peripheral
            uint8_t numBytes = CRUMBS_MESSAGE_SIZE;
            Wire.requestFrom(targetAddress, numBytes);

            // Allow some time for the peripheral to send the response
            delay(50);

            uint8_t responseBuffer[CRUMBS_MESSAGE_SIZE];
            int index = 0;
            while (Wire.available() && index < CRUMBS_MESSAGE_SIZE)
            {
                responseBuffer[index++] = Wire.read();
            }

            Serial.print(F("Controller: Received "));
            Serial.print(index);
            Serial.println(F(" bytes from peripheral."));

            // Attempt to decode the received response
            CRUMBSMessage response;
            if (crumbsController.decodeMessage(responseBuffer, index, response))
            {
                Serial.println(F("Controller: Decoded response:"));
                Serial.print(F("typeID: "));
                Serial.println(response.typeID);
                Serial.print(F("commandType: "));
                Serial.println(response.commandType);
                Serial.print(F("data: "));
                for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
                {
                    Serial.print(response.data[i]);
                    Serial.print(F(" "));
                }
                Serial.println();
                Serial.print(F("crc8: 0x"));
                Serial.println(response.crc8, HEX);
                reportCrcStatus(F("request decode"));
            }
            else
            {
                Serial.println(F("Controller: Failed to decode response."));
                reportCrcStatus(F("request decode"));
            }

            return; // Exit the function after handling the request command.
        }

        // Otherwise, assume the input is a comma-separated message.
        uint8_t targetAddress;
        CRUMBSMessage message;
        bool parseSuccess = parseSerialInput(input, targetAddress, message);

        if (parseSuccess)
        {
            // Send the message to the specified target address
            crumbsController.sendMessage(message, targetAddress);
            Serial.println(F("Controller: Message sent based on serial input."));
            reportCrcStatus(F("send"));
        }
        else
        {
            Serial.println(F("Controller: Failed to parse serial input. Please check the format."));
        }
    }
}

/**
 * @brief Parses a comma-separated serial input string into a target address and CRUMBSMessage.
 *
 * @param input The input string from serial.
 * @param targetAddress Reference to store the parsed I2C address.
 * @param message Reference to store the parsed CRUMBSMessage.
 * @return true If parsing was successful.
 * @return false If parsing failed due to incorrect format or insufficient fields.
 */
bool parseSerialInput(const String &input, uint8_t &targetAddress, CRUMBSMessage &message)
{
    int fieldCount = 0; /**< Number of fields parsed */
    int lastComma = 0;  /**< Position of the last comma */

    // Initialize message fields to default values
    message.typeID = 0;
    message.commandType = 0;
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        message.data[i] = 0.0f;
    }
    message.crc8 = 0;

    // Iterate through the input string to parse fields
    for (int i = 0; i <= input.length(); i++) // <= to capture the last field
    {
        if (i == input.length() || input[i] == ',')
        {
            String value = input.substring(lastComma, i);
            lastComma = i + 1;

            switch (fieldCount)
            {
            case 0:
                targetAddress = (uint8_t)value.toInt(); /**< Parse I2C address */
                break;
            case 1:
                message.typeID = (uint8_t)value.toInt(); /**< Parse typeID */
                break;
            case 2:
                message.commandType = (uint8_t)value.toInt(); /**< Parse commandType */
                break;
            case 3:
                message.data[0] = value.toFloat(); /**< Parse data0 */
                break;
            case 4:
                message.data[1] = value.toFloat(); /**< Parse data1 */
                break;
            case 5:
                message.data[2] = value.toFloat(); /**< Parse data2 */
                break;
            case 6:
                message.data[3] = value.toFloat(); /**< Parse data3 */
                break;
            case 7:
                message.data[4] = value.toFloat(); /**< Parse data4 */
                break;
            case 8:
                message.data[5] = value.toFloat(); /**< Parse data5 */
                break;
            case 9:
                message.data[6] = value.toFloat(); /**< Parse data6 */
                break;
            default:
                // Extra fields are ignored
                break;
            }
            fieldCount++;
        }
    }

    // Check if the required number of fields are parsed
    if (fieldCount < (3 + CRUMBS_DATA_LENGTH))
    {
        Serial.println(F("Controller: Not enough fields in input."));
        return false;
    }

    // Debugging output to verify parsed message
    Serial.println(F("Controller: Parsed input into CRUMBSMessage and target address."));
    Serial.print(F("Target Address: 0x"));
    Serial.println(targetAddress, HEX);
    Serial.print(F("Parsed Message -> typeID: "));
    Serial.print(message.typeID);
    Serial.print(F(", commandType: "));
    Serial.print(message.commandType);
    Serial.print(F(", data: "));
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        Serial.print(message.data[i]);
        Serial.print(F(" "));
    }
    Serial.println();
    Serial.println(F("Note: CRC will be calculated during encoding."));

    return true;
}
