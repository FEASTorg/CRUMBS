#include <Arduino.h>
#include <Wire.h>
#include <CRUMBS.h>

extern CRUMBS crumbsSlice;
extern LedChannel ledChannels[3];
extern MessageSummary lastCommand;
extern MessageSummary lastResponse;

extern void drawDisplay();
extern void applyCommand(const CRUMBSMessage &message);
extern void setOk();
extern void setError();

// Receive message from controller
void handleMessage(CRUMBSMessage &message)
{
    Serial.print(F("Command from controller | ratios="));
    Serial.print(message.data[0], 3);
    Serial.print(' ');
    Serial.print(message.data[1], 3);
    Serial.print(' ');
    Serial.print(message.data[2], 3);
    Serial.print(F(" period_ms="));
    Serial.println(static_cast<unsigned long>(message.data[3]));

    applyCommand(message);

    lastCommand.valid = true;
    lastCommand.message = message;

    setOk();
    drawDisplay();
}

// Respond to controller request
void handleRequest()
{
    CRUMBSMessage response = {};
    response.typeID = 1;
    response.commandType = 0;
    response.data[0] = ledChannels[0].ratio;
    response.data[1] = ledChannels[1].ratio;
    response.data[2] = ledChannels[2].ratio;
    response.data[3] = static_cast<float>(ledChannels[0].periodMs);
    response.data[4] = millis() / 1000.0f;
    response.data[5] = 0.0f;
    response.data[6] = 0.0f;

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
