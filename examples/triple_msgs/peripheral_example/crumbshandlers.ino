#include <Arduino.h>
#include <Wire.h>
#include <CRUMBS.h>

extern CRUMBS crumbsSlice;

extern struct LastMsg
{
    uint8_t typeID;
    uint8_t commandType;
    float data[CRUMBS_DATA_LENGTH];
    uint8_t crc8;
    bool valid;
} lastRx;

extern void drawDisplay();
extern void applyCommand(const CRUMBSMessage &message);
extern void setOk();
extern void setError();

// Receive message from controller
void handleMessage(CRUMBSMessage &message)
{
    Serial.println(F("Slice: message received."));
    Serial.print(F("typeID="));
    Serial.print(m.typeID);
    Serial.print(F(" cmd="));
    Serial.print(m.commandType);
    Serial.print(F(" data: "));
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        Serial.print(m.data[i], 3);
        Serial.print(' ');
    }
    Serial.print(F(" crc=0x"));
    Serial.println(m.crc8, HEX);

    // Cache last RX for display
    lastRx.typeID = m.typeID;
    lastRx.commandType = m.commandType;
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
        lastRx.data[i] = m.data[i];
    lastRx.crc8 = m.crc8;
    lastRx.valid = true;

    pulseActivity();
    setOk();

    // Handle command types
    switch (m.commandType)
    {
    case 0: // Request format/config: no immediate action, handled by onRequest
        Serial.println(F("Slice: command=REQUEST format/status."));
        break;

    case 1: // Set parameters (map floats to LED behaviors)
        Serial.println(F("Slice: command=SET parameters."));
        applyDataToChannels(m);
        break;

    default:
        Serial.println(F("Slice: Unknown command."));
        break;
    }

    drawDisplay();
}

// Respond to controller request
void handleRequest()
{
    Serial.println(F("Slice: request event -> sending status"));
    pulseActivity();

    CRUMBSMessage r;
    r.typeID = 1;      // slice type
    r.commandType = 0; // status response
    // Example status payload: echo last ratios for three channels and simple telemetry
    // d0..d2: desired duty for G,Y,R; d3: period(ms/1000), d4: uptime(s), d5/d6: reserved
    r.data[0] = constrain((float)analogRead(A0) / 1023.0f, 0.0f, 1.0f); // sample source (or use chans[0].ratio)
    r.data[1] = 0.0f;
    r.data[2] = 0.0f;
    r.data[3] = (float)(millis() / 1000UL);
    r.data[4] = 0.0f;
    r.data[5] = 0.0f;
    r.data[6] = 0.0f;
    r.crc8 = 0;

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t bytes = crumbsSlice.encodeMessage(response, buffer, sizeof(buffer));
    if (bytes == 0)
    {
        Serial.println(F("Slice: failed to encode response."));
        setError();
        return;
    }

    response.crc8 = buffer[bytes - 1];
    size_t written = Wire.write(buffer, bytes);

    if (written == bytes)
    {
        Serial.println(F("Slice: response sent."));
        lastResponse.valid = true;
        lastResponse.message = response;
        setOk();
    }
    else
    {
        Serial.println(F("Slice: response write truncated."));
        setError();
    }
}
