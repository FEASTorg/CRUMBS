/**
 * @file basic_controller.ino
 * @brief Basic CRUMBS controller with key commands - Pair with basic_peripheral
 */

#include <crumbs.h>
#include <crumbs_arduino.h>
#include <string.h>
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

void send_store_data()
{
    crumbs_message_t msg;
    msg.type_id = TARGET_TYPE_ID;
    msg.opcode = CMD_STORE_DATA;
    msg.data_len = (uint8_t)sizeof(STORE_PAYLOAD);
    memcpy(msg.data, STORE_PAYLOAD, sizeof(STORE_PAYLOAD));

    crumbs_controller_send(&ctx, TARGET_ADDR, &msg, crumbs_arduino_wire_write, NULL);
    Serial.println("Sent: Store data");
}

void send_clear_data()
{
    crumbs_message_t msg;
    msg.type_id = TARGET_TYPE_ID;
    msg.opcode = CMD_CLEAR_DATA;
    msg.data_len = 0;

    crumbs_controller_send(&ctx, TARGET_ADDR, &msg, crumbs_arduino_wire_write, NULL);
    Serial.println("Sent: Clear data");
}

void query_version()
{
    // Step 1: Send SET_REPLY to request opcode 0x00
    crumbs_message_t query;
    query.type_id = TARGET_TYPE_ID;
    query.opcode = CRUMBS_CMD_SET_REPLY;
    query.data_len = 1;
    query.data[0] = QUERY_VERSION_OPCODE;

    crumbs_controller_send(&ctx, TARGET_ADDR, &query, crumbs_arduino_wire_write, NULL);
    delay(QUERY_DELAY_MS);

    // Step 2: Read the reply
    uint8_t buf[READ_BUFFER_LEN];
    int n = crumbs_arduino_read(NULL, TARGET_ADDR, buf, sizeof(buf), READ_TIMEOUT_US);

    if (n >= 4)
    {
        crumbs_message_t reply;
        if (crumbs_decode_message(buf, n, &reply, &ctx) == 0)
        {
            Serial.print("Version: ");
            Serial.print(reply.data[2]);
            Serial.print(".");
            Serial.print(reply.data[3]);
            Serial.print(".");
            Serial.println(reply.data[4]);
        }
    }
}

void query_stored_data()
{
    // Step 1: Send SET_REPLY to request opcode 0x80
    crumbs_message_t query;
    query.type_id = TARGET_TYPE_ID;
    query.opcode = CRUMBS_CMD_SET_REPLY;
    query.data_len = 1;
    query.data[0] = QUERY_STORED_DATA_OPCODE;

    crumbs_controller_send(&ctx, TARGET_ADDR, &query, crumbs_arduino_wire_write, NULL);
    delay(QUERY_DELAY_MS);

    // Step 2: Read the reply
    uint8_t buf[READ_BUFFER_LEN];
    int n = crumbs_arduino_read(NULL, TARGET_ADDR, buf, sizeof(buf), READ_TIMEOUT_US);

    if (n >= 4)
    {
        crumbs_message_t reply;
        if (crumbs_decode_message(buf, n, &reply, &ctx) == 0)
        {
            Serial.print("Stored data (");
            Serial.print(reply.data_len);
            Serial.print(" bytes): ");
            for (int i = 0; i < reply.data_len; i++)
            {
                Serial.print(reply.data[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
        }
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    crumbs_arduino_init_controller(&ctx);

    Serial.println("=== Basic Controller ===");
    Serial.println("Commands:");
    Serial.println("  s - Store data");
    Serial.println("  c - Clear data");
    Serial.println("  v - Query version");
    Serial.println("  d - Query stored data");
}

void loop()
{
    heartbeat_tick();

    if (Serial.available())
    {
        char cmd = Serial.read();

        switch (cmd)
        {
        case CMD_KEY_STORE:
            send_store_data();
            break;
        case CMD_KEY_CLEAR:
            send_clear_data();
            break;
        case CMD_KEY_QUERY_VERSION:
            query_version();
            break;
        case CMD_KEY_QUERY_STORED:
            query_stored_data();
            break;
        }
    }
}
