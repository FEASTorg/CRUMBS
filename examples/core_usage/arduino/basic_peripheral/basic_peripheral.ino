/**
 * @file basic_peripheral.ino
 * @brief Basic CRUMBS peripheral - handles multiple commands
 */

#include <crumbs.h>
#include <crumbs_arduino.h>
#include "config.h"

crumbs_context_t ctx;
uint8_t stored_data[STORED_DATA_CAPACITY];
uint8_t stored_len = 0;
static uint32_t last_heartbeat_ms = 0;
static bool led_state = false;

static void heartbeat_tick()
{
    const uint32_t now = millis();
    if ((uint32_t)(now - last_heartbeat_ms) < HEARTBEAT_INTERVAL_MS)
        return;

    last_heartbeat_ms = now;
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state ? HIGH : LOW);
}

void on_message(crumbs_context_t *ctx, const crumbs_message_t *msg)
{
    Serial.print("RX: cmd=");
    Serial.print(msg->opcode);
    Serial.print(" len=");
    Serial.println(msg->data_len);

    // Store or process based on opcode
    switch (msg->opcode)
    {
    case CMD_STORE_DATA:
        if (msg->data_len <= sizeof(stored_data))
        {
            memcpy(stored_data, msg->data, msg->data_len);
            stored_len = msg->data_len;
            Serial.println("Data stored");
        }
        break;

    case CMD_CLEAR_DATA:
        stored_len = 0;
        Serial.println("Data cleared");
        break;

    default:
        Serial.println("Unknown command");
        break;
    }
}

void on_request(crumbs_context_t *ctx, crumbs_message_t *reply)
{
    uint8_t requested_op = ctx->requested_opcode;

    reply->type_id = DEVICE_TYPE_ID;
    reply->opcode = requested_op;

    if (requested_op == QUERY_VERSION_OPCODE)
    {
        // Version info
        reply->data_len = 5;
        reply->data[0] = CRUMBS_VERSION & 0xFF;        // LSB first (little-endian)
        reply->data[1] = (CRUMBS_VERSION >> 8) & 0xFF; // MSB second
        reply->data[2] = MODULE_VERSION_MAJOR;
        reply->data[3] = MODULE_VERSION_MINOR;
        reply->data[4] = MODULE_VERSION_PATCH;
    }
    else if (requested_op == QUERY_STORED_DATA_OPCODE)
    {
        // Return stored data
        reply->data_len = stored_len;
        memcpy(reply->data, stored_data, stored_len);
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    crumbs_arduino_init_peripheral(&ctx, DEVICE_ADDR);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
    Serial.print("Basic peripheral ready at 0x");
    Serial.println(DEVICE_ADDR, HEX);
}

void loop()
{
    // All processing in callbacks.
    heartbeat_tick();
    delay(1);
}
