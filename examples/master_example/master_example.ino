/**
 * @file crumbs_master_test.ino
 * @brief CRUMBS Master example sketch to send messages to a CRUMBS Slice.
 */

#define CRUMBS_DEBUG
#include <CRUMBS.h>
#include <Wire.h>

/**
 * @brief Instantiate CRUMBS as Master.
 * 
 * @note Master mode is indicated by passing 'true'.
 */
CRUMBS crumbsMaster(true); // Master mode

/**
 * @brief Maximum expected input length for serial commands.
 */
#define MAX_INPUT_LENGTH 60

/**
 * @brief Initializes the Master device, sets up serial communication, and provides usage instructions.
 */
void setup()
{
    Serial.begin(115200); /**< Initialize serial communication at 115200 baud rate */
    while (!Serial)
    {
        delay(10); // Wait for Serial Monitor to open
    }
    crumbsMaster.begin(); /**< Initialize CRUMBS communication */
    Serial.println(F("Master ready. Enter messages in the format:"));
    Serial.println(F("address,typeID,unitID,commandType,data0,data1,data2,data3,data4,data5,errorFlags"));
    Serial.println(F("Example Commands:"));
    Serial.println(F("1. Send Message:"));
    Serial.println(F("   8,1,1,1,75.0,1.0,0.0,65.0,2.0,1.0,0")); // Example: address 8, typeID 1, unitID 1, commandType 1, data..., errorFlags 0
    Serial.println(F("2. Request Data:"));
    Serial.println(F("   8,1,1,0,0.0,0.0,0.0,0.0,0.0,0.0,0")); // Example: commandType 0 indicates a request
}

/**
 * @brief Main loop that listens for serial input, parses commands, and sends CRUMBSMessages to the specified Slice.
 */
void loop()
{
    // Listen for serial input to send commands or request data
    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n'); /**< Read input until newline */
        input.trim(); // Remove any trailing newline or carriage return characters

        Serial.print(F("Master: Received input from serial: "));
        Serial.println(input);

        // Check if the input is empty
        if (input.length() == 0)
        {
            Serial.println(F("Master: Empty input received. Ignoring."));
            return;
        }

        // Parse the input string into a CRUMBSMessage and target address
        uint8_t targetAddress;
        CRUMBSMessage message;
        bool parseSuccess = parseSerialInput(input, targetAddress, message);

        if (parseSuccess)
        {
            // Send the message to the specified target address
            crumbsMaster.sendMessage(message, targetAddress);
            Serial.println(F("Master: Message sent based on serial input."));
        }
        else
        {
            Serial.println(F("Master: Failed to parse serial input. Please check the format."));
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
    int lastComma = 0; /**< Position of the last comma */

    // Initialize message fields to default values
    message.typeID = 0;
    message.unitID = 0;
    message.commandType = 0;
    for (int i = 0; i < 6; i++)
    {
        message.data[i] = 0.0f;
    }
    message.errorFlags = 0;

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
                    message.unitID = (uint8_t)value.toInt(); /**< Parse unitID */
                    break;
                case 3:
                    message.commandType = (uint8_t)value.toInt(); /**< Parse commandType */
                    break;
                case 4:
                    message.data[0] = value.toFloat(); /**< Parse data0 */
                    break;
                case 5:
                    message.data[1] = value.toFloat(); /**< Parse data1 */
                    break;
                case 6:
                    message.data[2] = value.toFloat(); /**< Parse data2 */
                    break;
                case 7:
                    message.data[3] = value.toFloat(); /**< Parse data3 */
                    break;
                case 8:
                    message.data[4] = value.toFloat(); /**< Parse data4 */
                    break;
                case 9:
                    message.data[5] = value.toFloat(); /**< Parse data5 */
                    break;
                case 10:
                    message.errorFlags = (uint8_t)value.toInt(); /**< Parse errorFlags */
                    break;
                default:
                    // Extra fields are ignored
                    break;
            }
            fieldCount++;
        }
    }

    // Check if the required number of fields are parsed
    if (fieldCount < 4)
    {
        Serial.println(F("Master: Not enough fields in input."));
        return false;
    }

    // Debugging output to verify parsed message
    Serial.println(F("Master: Parsed input into CRUMBSMessage and target address."));
    Serial.print(F("Target Address: "));
    Serial.println(targetAddress, HEX);
    Serial.print(F("Parsed Message -> typeID: "));
    Serial.print(message.typeID);
    Serial.print(F(", unitID: "));
    Serial.print(message.unitID);
    Serial.print(F(", commandType: "));
    Serial.print(message.commandType);
    Serial.print(F(", data: "));
    for (int i = 0; i < 6; i++)
    {
        Serial.print(message.data[i]);
        Serial.print(F(" "));
    }
    Serial.print(F(", errorFlags: "));
    Serial.println(message.errorFlags);

    return true;
}
