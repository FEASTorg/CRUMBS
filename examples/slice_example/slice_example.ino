#define CRUMBS_DEBUG
#include <CRUMBS.h>

CRUMBS crumbsSlice(false, 0x08); // Slice mode with address 0x08

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    crumbsSlice.begin();
    crumbsSlice.onReceive(handleMessage);
    crumbsSlice.onRequest(handleRequest);
    Serial.println("Slice ready and listening for messages...");
}

void loop() {
    // The slice does not send messages automatically; it only responds to master's requests.
}

// Function to handle messages received from the master
void handleMessage(CRUMBSMessage& message) {
    Serial.println("Slice: Received Message:");
    Serial.print("Slice ID: "); Serial.println(message.sliceID);
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

// Function to handle master's request for data
void handleRequest() {
    CRUMBS_DEBUG_PRINTLN("Slice: Master requested data, sending response...");

    // Prepare response message
    CRUMBSMessage responseMessage;
    responseMessage.sliceID = crumbsSlice.getAddress();
    responseMessage.typeID = 2;           // Example Type ID
    responseMessage.commandType = 5;      // Example Command Type
    responseMessage.data[0] = 42.0;       // Example Data
    responseMessage.data[1] = 0.0;
    responseMessage.data[2] = 0.0;
    responseMessage.data[3] = 0.0;
    responseMessage.errorFlags = 0;       // No errors

    uint8_t buffer[20];
    size_t encodedSize = crumbsSlice.encodeMessage(responseMessage, buffer, sizeof(buffer));

    if (encodedSize == 0) {
        CRUMBS_DEBUG_PRINTLN("Slice: Failed to encode response message.");
        return;
    }

    Wire.write(buffer, encodedSize);
    CRUMBS_DEBUG_PRINTLN("Slice: Response message sent.");
}
