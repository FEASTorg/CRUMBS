#define CRUMBS_DEBUG
#include <CRUMBS.h>
#include <Wire.h>

// Instantiate CRUMBS as Master
CRUMBS crumbsMaster(true); // Master mode

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }
    crumbsMaster.begin();
    Serial.println(F("Master ready. Enter messages in the format: sliceID,typeID,commandType,data0,data1,data2,data3,data4,data5,errorFlags"));
    Serial.println(F("Example Commands:"));
    Serial.println(F("1. Send Message:"));
    Serial.println(F("   8,1,1,75.0,1.0,0.0,65.0,2.0,1.0,0")); // Set Heater Parameters
    Serial.println(F("2. Set PID Tunings:"));
    Serial.println(F("   8,1,2,2.0,5.0,1.0,2.0,5.0,1.0,0"));   // Set PID Tunings
    Serial.println(F("3. Request Data:"));
    Serial.println(F("   request"));
}

void loop()
{
    // Listen for serial input to send commands or request data
    if (Serial.available())
    {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Remove any trailing newline characters

        Serial.print(F("Master: Received input from serial: "));
        Serial.println(input);

        if (input.startsWith("request"))
        {
            // Request data from the slice
            Serial.println(F("Master: Requesting data from slice..."));
            uint8_t targetAddress = 0x08;                                  // Address of the slice
            Wire.requestFrom(targetAddress, (uint8_t)CRUMBS_MESSAGE_SIZE); // Request 28 bytes

            uint8_t buffer[CRUMBS_MESSAGE_SIZE];
            size_t index = 0;
            while (Wire.available() && index < CRUMBS_MESSAGE_SIZE)
            {
                buffer[index++] = Wire.read();
            }

            CRUMBSMessage receivedMessage;
            if (crumbsMaster.decodeMessage(buffer, index, receivedMessage))
            {
                Serial.println(F("Master: Received Message from Slice:"));
                Serial.print(F("Slice ID: "));
                Serial.println(receivedMessage.sliceID);
                Serial.print(F("Type ID: "));
                Serial.println(receivedMessage.typeID);
                Serial.print(F("Command Type: "));
                Serial.println(receivedMessage.commandType);
                Serial.print(F("Data: "));
                for (int i = 0; i < 6; i++)
                {
                    Serial.print(receivedMessage.data[i]);
                    Serial.print(F(" "));
                }
                Serial.println();
                Serial.print(F("Error Flags: "));
                Serial.println(receivedMessage.errorFlags);
            }
            else
            {
                Serial.println(F("Master: Failed to decode message from slice."));
            }
        }
        else
        {
            // Send command to slice
            CRUMBSMessage message = parseSerialInput(input);
            uint8_t targetAddress = message.sliceID; // Use sliceID as the target address
            crumbsMaster.sendMessage(message, targetAddress);
            Serial.println(F("Master: Message sent based on serial input"));
        }
    }
}

// Function to parse the serial input and create a CRUMBSMessage
CRUMBSMessage parseSerialInput(const String &input)
{
    CRUMBSMessage message;
    int index = 0;
    int lastComma = 0;

    // Initialize all fields to default values
    message.sliceID = 0x08; // Targeting Relay Heater slice
    message.typeID = 0x01;  // Type ID for Relay Heater
    message.commandType = 0; // Default command type
    for (int i = 0; i < 6; i++)
    {
        message.data[i] = 0.0;
    }
    message.errorFlags = 0;

    // Parse each value in the comma-separated input string
    for (int i = 0; i <= input.length(); i++) { // <= to capture last character
        if (i == input.length() || input[i] == ',') {
            String value = input.substring(lastComma, (i == input.length() - 1 && input[i] != ',') ? i + 1 : i);
            lastComma = i + 1;

            switch (index) {
                case 0:
                    message.commandType = value.toInt();
                    break;
                case 1:
                    message.data[0] = value.toFloat();
                    break;
                case 2:
                    message.data[1] = value.toFloat();
                    break;
                case 3:
                    message.data[2] = value.toFloat();
                    break;
                case 4:
                    message.data[3] = value.toFloat();
                    break;
                case 5:
                    message.data[4] = value.toFloat();
                    break;
                case 6:
                    message.data[5] = value.toFloat();
                    break;
                case 7:
                    message.errorFlags = value.toInt();
                    break;
                default:
                    break;
            }
            index++;
        }
    }

    // Debugging output
    Serial.println(F("Master: Parsed input into CRUMBSMessage"));
    Serial.print(F("Parsed Message -> sliceID: "));
    Serial.print(message.sliceID);
    Serial.print(F(", typeID: "));
    Serial.print(message.typeID);
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

    return message;
}
