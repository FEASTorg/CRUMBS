#include <Arduino.h>
#include <Wire.h>
#include "crumbs_arduino.h"

extern crumbs_context_t crumbs_ctx;

extern crumbs_message_t lastRxMessage;
extern bool lastRxValid;

extern void drawDisplay();
extern void applyCommand(const crumbs_message_t &message);
extern void setOk();
extern void setError();

// Receive message from controller
// Callback invoked when a complete, valid message is received.
void handleMessage(crumbs_context_t *ctx, const crumbs_message_t *m)
{
    Serial.println(F("Slice: message received."));
    Serial.print(F("typeID="));
    Serial.print(m->type_id);
    Serial.print(F(" cmd="));
    Serial.print(m->command_type);
    Serial.print(F(" data_len="));
    Serial.print(m->data_len);
    Serial.print(F(" data: "));
    for (int i = 0; i < m->data_len && i < 8; i++)
    {
        Serial.print(F("0x"));
        if (m->data[i] < 0x10) Serial.print('0');
        Serial.print(m->data[i], HEX);
        Serial.print(' ');
    }
    Serial.print(F(" crc=0x"));
    Serial.println(m->crc8, HEX);

    // Cache last RX for display
    lastRxMessage = *m;
    lastRxValid = true;

    pulseActivity();
    setOk();

    // Handle command types
    switch (m->command_type)
    {
    case 0: // Request format/config: no immediate action, handled by onRequest
        Serial.println(F("Slice: command=REQUEST format/status."));
        break;

    case 1: // Set parameters (parse payload bytes as LED behaviors)
        Serial.println(F("Slice: command=SET parameters."));
        applyDataToChannels(*m);
        break;

    default:
        Serial.println(F("Slice: Unknown command."));
        break;
    }

    drawDisplay();
}

// Respond to controller request
// Called when a controller requests data from this peripheral. Fill @p response.
void handleRequest(crumbs_context_t *ctx, crumbs_message_t *response)
{
    Serial.println(F("Slice: request event -> sending status"));
    pulseActivity();
    // Build the response message; core will encode it & send on the bus.
    response->type_id = 1;      // slice type
    response->command_type = 0; // status response

    // Example status payload: 4 floats (16 bytes)
    // float[0]: sample analog value, float[1-2]: reserved, float[3]: uptime in seconds
    float payload[4];
    payload[0] = constrain((float)analogRead(A0) / 1023.0f, 0.0f, 1.0f);
    payload[1] = 0.0f;
    payload[2] = 0.0f;
    payload[3] = (float)(millis() / 1000UL);

    response->data_len = sizeof(payload);
    memcpy(response->data, payload, sizeof(payload));
    response->crc8 = 0; // filled by encoder

    // Mark last response for display
    lastResponseMessage = *response;
    lastResponseValid = true;
    setOk();
}
