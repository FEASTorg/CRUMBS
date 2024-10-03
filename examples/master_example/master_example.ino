#define CRUMBS_DEBUG
#include <CRUMBS.h>

CRUMBS crumbsMaster(true); // Master mode

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    crumbsMaster.begin();
    Serial.println("Master ready. Enter messages in the format: sliceID,typeID,commandType,data1,data2,data3,data4,errorFlags");
}

void loop() {
    // Wait for serial input to send commands or request data
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        Serial.print("Master: Received input from serial: ");
        Serial.println(input);

        if (input.startsWith("request")) {
            // Request data from the slice
            Serial.println("Master: Requesting data from slice...");
            uint8_t targetAddress = 0x08; // Address of the slice
            Wire.requestFrom(targetAddress, (uint8_t)64); // Request up to 64 bytes

            uint8_t buffer[64];
            size_t index = 0;
            while (Wire.available()) {
                buffer[index++] = Wire.read();
            }

            CRUMBSMessage receivedMessage;
            if (crumbsMaster.decodeMessage(buffer, index, receivedMessage)) {
                Serial.println("Master: Received Message from Slice:");
                Serial.print("Slice ID: "); Serial.println(receivedMessage.sliceID);
                Serial.print("Type ID: "); Serial.println(receivedMessage.typeID);
                Serial.print("Command Type: "); Serial.println(receivedMessage.commandType);
                Serial.print("Data: ");
                for (int i = 0; i < 4; i++) {
                    Serial.print(receivedMessage.data[i]);
                    Serial.print(" ");
                }
                Serial.println();
                Serial.print("Error Flags: "); Serial.println(receivedMessage.errorFlags);
            } else {
                Serial.println("Master: Failed to decode message from slice.");
            }
        } else {
            // Send command to slice
            CRUMBSMessage message = parseSerialInput(input);
            uint8_t targetAddress = message.sliceID; // Use sliceID as the target address
            crumbsMaster.sendMessage(message, targetAddress);
            Serial.println("Master: Message sent based on serial input");
        }
    }
}

// Function to parse the serial input and create a CRUMBSMessage
CRUMBSMessage parseSerialInput(const String& input) {
    CRUMBSMessage message;
    int index = 0;
    int lastComma = 0;

    // Parse each value in the comma-separated input string
    for (int i = 0; i < input.length(); i++) {
        if (input[i] == ',' || i == input.length() - 1) {
            String value = input.substring(lastComma, (i == input.length() - 1) ? i + 1 : i);
            lastComma = i + 1;

            switch (index) {
                case 0: message.sliceID = value.toInt(); break;
                case 1: message.typeID = value.toInt(); break;
                case 2: message.commandType = value.toInt(); break;
                case 3: message.data[0] = value.toFloat(); break;
                case 4: message.data[1] = value.toFloat(); break;
                case 5: message.data[2] = value.toFloat(); break;
                case 6: message.data[3] = value.toFloat(); break;
                case 7: message.errorFlags = value.toInt(); break;
            }
            index++;
        }
    }

    Serial.println("Master: Parsed input into CRUMBSMessage");
    Serial.print("Parsed Message -> sliceID: ");
    Serial.print(message.sliceID);
    Serial.print(", typeID: ");
    Serial.print(message.typeID);
    Serial.print(", commandType: ");
    Serial.print(message.commandType);
    Serial.print(", data: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(message.data[i]);
        Serial.print(" ");
    }
    Serial.print(", errorFlags: ");
    Serial.println(message.errorFlags);

    return message;
}
