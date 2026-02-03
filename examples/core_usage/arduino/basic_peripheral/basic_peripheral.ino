/**
 * @file basic_peripheral.ino
 * @brief Basic CRUMBS peripheral - handles multiple commands
 */

#include <crumbs.h>
#include <crumbs_arduino.h>

crumbs_context_t ctx;
uint8_t stored_data[10];
uint8_t stored_len = 0;

void on_message(crumbs_context_t *ctx, const crumbs_message_t *msg)
{
    Serial.print("RX: cmd=");
    Serial.print(msg->opcode);
    Serial.print(" len=");
    Serial.println(msg->data_len);

    // Store or process based on opcode
    switch (msg->opcode)
    {
    case 0x01: // Store data
        if (msg->data_len <= sizeof(stored_data))
        {
            memcpy(stored_data, msg->data, msg->data_len);
            stored_len = msg->data_len;
            Serial.println("Data stored");
        }
        break;

    case 0x02: // Clear data
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

    reply->type_id = 1;
    reply->opcode = requested_op;

    if (requested_op == 0x00)
    {
        // Version info
        reply->data_len = 5;
        reply->data[0] = CRUMBS_VERSION & 0xFF;        // LSB first (little-endian)
        reply->data[1] = (CRUMBS_VERSION >> 8) & 0xFF; // MSB second
        reply->data[2] = 1;                            // Major
        reply->data[3] = 0;                            // Minor
        reply->data[4] = 0;                            // Patch
    }
    else if (requested_op == 0x80)
    {
        // Return stored data
        reply->data_len = stored_len;
        memcpy(reply->data, stored_data, stored_len);
    }
}

void setup()
{
    Serial.begin(115200);
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
    Serial.println("Basic peripheral ready at 0x08");
}

void loop()
{
    // All processing in callbacks
}
