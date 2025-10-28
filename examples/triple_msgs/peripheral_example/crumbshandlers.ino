#include <Arduino.h>
#include <Wire.h>

extern CRUMBS crumbsSlice;

extern struct LastMsg
{
    uint8_t typeID;
    uint8_t commandType;
    float data[6];
    uint8_t errorFlags;
    bool valid;
} lastRx;

extern void drawDisplay();
extern void setOk();
extern void setError();
extern void pulseActivity();
extern void applyDataToChannels(const CRUMBSMessage &m);

// Receive message from controller
void handleMessage(CRUMBSMessage &m)
{
    Serial.println(F("Slice: message received."));
    Serial.print(F("typeID="));
    Serial.print(m.typeID);
    Serial.print(F(" cmd="));
    Serial.print(m.commandType);
    Serial.print(F(" data: "));
    for (int i = 0; i < 6; i++)
    {
        Serial.print(m.data[i], 3);
        Serial.print(' ');
    }
    Serial.print(F(" err="));
    Serial.println(m.errorFlags);

    // Cache last RX for display
    lastRx.typeID = m.typeID;
    lastRx.commandType = m.commandType;
    for (int i = 0; i < 6; i++)
        lastRx.data[i] = m.data[i];
    lastRx.errorFlags = m.errorFlags;
    lastRx.valid = true;

    pulseActivity();

    // Error handling
    if (m.errorFlags != 0)
        setError();
    else
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
    // Example status payload: echo last ratios for three chans and basic flags
    // d0..d2: desired duty for G,Y,R; d3: period(ms/1000), d4: uptime(s), d5: reserved
    r.data[0] = constrain((float)analogRead(A0) / 1023.0f, 0.0f, 1.0f); // sample source (or use chans[0].ratio)
    r.data[1] = 0.0f;
    r.data[2] = 0.0f;
    r.data[3] = (float)(millis() / 1000UL);
    r.data[4] = 0.0f;
    r.data[5] = 0.0f;
    r.errorFlags = 0;

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t n = crumbsSlice.encodeMessage(r, buffer, sizeof(buffer));
    if (n == 0)
    {
        Serial.println(F("Slice: encode status failed"));
        return;
    }
    Wire.write(buffer, n);
}
