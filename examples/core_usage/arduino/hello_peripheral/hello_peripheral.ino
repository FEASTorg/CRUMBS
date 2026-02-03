/**
 * @file hello_peripheral.ino
 * @brief Minimal CRUMBS peripheral example - Start here!
 */

#include <crumbs.h>
#include <crumbs_arduino.h>

crumbs_context_t ctx;
uint8_t counter = 0;

void on_message(crumbs_context_t *ctx, const crumbs_message_t *msg)
{
    Serial.print("RX cmd=");
    Serial.println(msg->opcode);
    counter++;
}

void on_request(crumbs_context_t *ctx, crumbs_message_t *reply)
{
    reply->type_id = 1;
    reply->opcode = 0;
    reply->data_len = 1;
    reply->data[0] = counter;
}

void setup()
{
    Serial.begin(115200);
    crumbs_arduino_init_peripheral(&ctx, 0x08);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
    Serial.println("Hello peripheral at 0x08");
}

void loop() {}
