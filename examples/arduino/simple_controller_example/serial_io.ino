namespace
{
    uint32_t lastCrcReport = 0;
    bool lastCrcValid = true;

    void reportCrcStatus(const __FlashStringHelper *context)
    {
        const uint32_t current = crumbs_get_crc_error_count(&crumbsController);
        const bool lastValid = crumbs_last_crc_ok(&crumbsController);
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

/**
 * @brief Handles serial input from the user to send crumbs_message_ts to a specified target address.
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
            /* Ensure we select the int,int overload explicitly on all cores. */
            Wire.requestFrom((int)targetAddress, (int)numBytes);

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
            crumbs_message_t response;

            if (crumbs_decode_message(responseBuffer, index, &response, &crumbsController))
            {
                Serial.println(F("Controller: Decoded response:"));
                Serial.print(F("type_id: "));
                Serial.println(response.type_id);
                Serial.print(F("command_type: "));
                Serial.println(response.command_type);
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

        // Allow user to run a CRUMBS-specific scan via "scan" or "scan strict"
        if (input.startsWith("scan"))
        {
            bool strictMode = false;
            if (input.indexOf("strict") >= 0)
                strictMode = true;

            Serial.print(F("Controller: Performing CRUMBS scan (strict="));
            Serial.print(strictMode ? F("true") : F("false"));
            Serial.println(F(")"));

            uint8_t found[32];
            int n = crumbs_controller_scan_for_crumbs(
                &crumbsController,
                0x03,
                0x77,
                strictMode ? 1 : 0,
                crumbs_arduino_wire_write,
                crumbs_arduino_read,
                &Wire,
                found,
                sizeof(found),
                50000 /* timeout_us */
            );

            if (n < 0)
            {
                Serial.print(F("Controller: scan failed ("));
                Serial.print(n);
                Serial.println(F(")"));
            }
            else if (n == 0)
            {
                Serial.println(F("Controller: no CRUMBS devices found"));
            }
            else
            {
                Serial.print(F("Controller: found "));
                Serial.print(n);
                Serial.println(F(" CRUMBS device(s):"));
                for (int i = 0; i < n; ++i)
                {
                    Serial.print(F("  0x"));
                    if (found[i] < 16)
                        Serial.print(F("0"));
                    Serial.println(found[i], HEX);
                }
            }

            return;
        }

        // Otherwise, assume the input is a comma-separated message.
        uint8_t targetAddress;
        crumbs_message_t message;
        bool parseSuccess = parseSerialInput(input, targetAddress, message);

        if (parseSuccess)
        {
            // Send the message to the specified target address
            crumbs_controller_send(
                &crumbsController,
                targetAddress,
                &message,
                crumbs_arduino_wire_write,
                &Wire);
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
 * @brief Parses a comma-separated serial input string into a target address and crumbs_message_t.
 *
 * @param input The input string from serial.
 * @param targetAddress Reference to store the parsed I2C address.
 * @param message Reference to store the parsed crumbs_message_t.
 * @return true If parsing was successful.
 * @return false If parsing failed due to incorrect format or insufficient fields.
 */
bool parseSerialInput(const String &input, uint8_t &targetAddress, crumbs_message_t &message)
{
    int fieldCount = 0; /**< Number of fields parsed */
    int lastComma = 0;  /**< Position of the last comma */

    // Initialize message fields to default values
    message.type_id = 0;
    message.command_type = 0;
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
                message.type_id = (uint8_t)value.toInt(); /**< Parse type_id */
                break;
            case 2:
                message.command_type = (uint8_t)value.toInt(); /**< Parse command_type */
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
    Serial.println(F("Controller: Parsed input into crumbs_message_t and target address."));
    Serial.print(F("Target Address: 0x"));
    Serial.println(targetAddress, HEX);
    Serial.print(F("Parsed Message -> type_id: "));
    Serial.print(message.type_id);
    Serial.print(F(", command_type: "));
    Serial.print(message.command_type);
    Serial.print(F(", data: "));
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        Serial.print(message.data[i]);
        Serial.print(F(" "));
    }
    Serial.println();
    Serial.println(F("CRC calculated during encoding."));

    return true;
}
