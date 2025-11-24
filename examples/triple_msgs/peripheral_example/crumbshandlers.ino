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
    Serial.print(F(" data: "));
    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        Serial.print(m->data[i], 3);
        Serial.print(' ');
    }
    Serial.print(F(" crc=0x"));
    Serial.println(m.crc8, HEX);

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

    case 1: // Set parameters (map floats to LED behaviors)
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
    // Example status payload: echo last ratios for three channels and simple telemetry
    // d0..d2: desired duty for G,Y,R; d3: period(ms/1000), d4: uptime(s), d5/d6: reserved
    response->data[0] = constrain((float)analogRead(A0) / 1023.0f, 0.0f, 1.0f); // sample source (or use chans[0].ratio)
    response->data[1] = 0.0f;
    response->data[2] = 0.0f;
    response->data[3] = (float)(millis() / 1000UL);
    response->data[4] = 0.0f;
    response->data[5] = 0.0f;
    response->data[6] = 0.0f;
    response->crc8 = 0;
    // Mark last response for display
    lastResponseMessage = *response;
    lastResponseValid = true;
    setOk();
}
