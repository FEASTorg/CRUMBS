/**
 * @file hello_controller.ino
 * @brief Minimal CRUMBS controller example - Pair with hello_peripheral
 */

#include <crumbs.h>
#include <crumbs_arduino.h>
#include "config.h"

crumbs_context_t ctx;
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

void send_hello()
{
    crumbs_message_t msg;
    msg.type_id = TARGET_TYPE_ID;
    msg.opcode = HELLO_OPCODE;
    msg.data_len = (uint8_t)sizeof(HELLO_PAYLOAD);
    msg.data[0] = HELLO_PAYLOAD[0];
    msg.data[1] = HELLO_PAYLOAD[1];

    crumbs_controller_send(&ctx, TARGET_ADDR, &msg, crumbs_arduino_wire_write, NULL);
    Serial.println("Sent!");
}

void request_data()
{
    // Step 1: Send SET_REPLY to tell peripheral what we want
    crumbs_message_t query;
    query.type_id = TARGET_TYPE_ID;
    query.opcode = CRUMBS_CMD_SET_REPLY;
    query.data_len = 1;
    query.data[0] = QUERY_TARGET_OPCODE;
    crumbs_controller_send(&ctx, TARGET_ADDR, &query, crumbs_arduino_wire_write, NULL);

    // Step 2: Read the reply
    delay(QUERY_DELAY_MS);
    uint8_t buf[READ_BUFFER_LEN];
    int n = crumbs_arduino_read(NULL, TARGET_ADDR, buf, sizeof(buf), READ_TIMEOUT_US);

    if (n > 0)
    {
        Serial.print("Received ");
        Serial.print(n);
        Serial.println(" bytes");
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    crumbs_arduino_init_controller(&ctx);
    Serial.println("Commands: s=send, r=request");
}

void loop()
{
    heartbeat_tick();

    if (Serial.available())
    {
        char c = Serial.read();
        if (c == CMD_SEND)
            send_hello();
        if (c == CMD_REQUEST)
            request_data();
    }
}
