// File: examples/CRUMBS_Test/crumbs_slice_test.ino

#define CRUMBS_DEBUG
#include <CRUMBS.h>

// Instantiate CRUMBS as a Slice (Slave) with I2C address 0x08
CRUMBS crumbsSlice(false, 0x08); // false = Slave mode, 0x08 = I2C address

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); } // Wait for Serial Monitor to open
    crumbsSlice.begin();
    crumbsSlice.onReceive(handleMessage);
    crumbsSlice.onRequest(handleRequest);
    Serial.println("Slice ready and listening for messages...");
}

void loop() {
    // The slice does not perform actions in loop; it responds to Master via callbacks
}

// Callback function to handle received messages from the Master
void handleMessage(CRUMBSMessage& message) {
    Serial.println("Slice: Received Message:");
    Serial.print("Slice ID: "); Serial.println(message.sliceID, HEX);
    Serial.print("Type ID: "); Serial.println(message.typeID);
    Serial.print("Command Type: "); Serial.println(message.commandType);
    Serial.print("Data: ");
    for (int i = 0; i < 4; i++) {
        Serial.print(message.data[i]);
        Serial.print(" ");
    }
    Serial.println();
    Serial.print("Error Flags: "); Serial.println(message.errorFlags);
    Serial.println("Slice: Message successfully processed by handleMessage callback");
}

// Callback function to handle Master's data requests
void handleRequest() {
    CRUMBS_DEBUG_PRINTLN("Slice: Master requested data, preparing response...");

    // Prepare response message
    CRUMBSMessage responseMessage;
    responseMessage.sliceID = crumbsSlice.getAddress(); // Slice's own address
    responseMessage.typeID = 2;           // Example Type ID (can be customized)
    responseMessage.commandType = 5;      // Example Command Type (e.g., status)
    responseMessage.data[0] = 42.0;       // Example Data (e.g., sensor reading)
    responseMessage.data[1] = 0.0;
    responseMessage.data[2] = 0.0;
    responseMessage.data[3] = 0.0;
    responseMessage.errorFlags = 0;       // No errors

    // Encode the message
    uint8_t buffer[20];
    size_t encodedSize = crumbsSlice.encodeMessage(responseMessage, buffer, sizeof(buffer));

    if (encodedSize == 0) {
        CRUMBS_DEBUG_PRINTLN("Slice: Failed to encode response message.");
        return;
    }

    // Send the encoded message back to the Master
    Wire.write(buffer, encodedSize);
    CRUMBS_DEBUG_PRINTLN("Slice: Response message sent.");
}
