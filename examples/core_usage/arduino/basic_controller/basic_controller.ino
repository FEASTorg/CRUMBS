/**
 * @file basic_controller.ino
 * @brief Basic CRUMBS controller with key commands - Pair with basic_peripheral
 */

#include <crumbs.h>
#include <crumbs_arduino.h>

crumbs_context_t ctx;

void send_store_data()
{
    crumbs_message_t msg;
    msg.type_id = 1;
    msg.opcode = 0x01; // Store command
    msg.data_len = 3;
    msg.data[0] = 0xAA;
    msg.data[1] = 0xBB;
    msg.data[2] = 0xCC;

    crumbs_controller_send(&ctx, 0x08, &msg, crumbs_arduino_wire_write, NULL);
    Serial.println("Sent: Store data");
}

void send_clear_data()
{
    crumbs_message_t msg;
    msg.type_id = 1;
    msg.opcode = 0x02; // Clear command
    msg.data_len = 0;

    crumbs_controller_send(&ctx, 0x08, &msg, crumbs_arduino_wire_write, NULL);
    Serial.println("Sent: Clear data");
}

void query_version()
{
    // Step 1: Send SET_REPLY to request opcode 0x00
    crumbs_message_t query;
    query.type_id = 1;
    query.opcode = 0xFE; // SET_REPLY
    query.data_len = 1;
    query.data[0] = 0x00; // Request version

    crumbs_controller_send(&ctx, 0x08, &query, crumbs_arduino_wire_write, NULL);
    delay(10);

    // Step 2: Read the reply
    uint8_t buf[32];
    int n = crumbs_arduino_read(NULL, 0x08, buf, sizeof(buf), 5000);

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
    query.type_id = 1;
    query.opcode = 0xFE; // SET_REPLY
    query.data_len = 1;
    query.data[0] = 0x80; // Request stored data

    crumbs_controller_send(&ctx, 0x08, &query, crumbs_arduino_wire_write, NULL);
    delay(10);

    // Step 2: Read the reply
    uint8_t buf[32];
    int n = crumbs_arduino_read(NULL, 0x08, buf, sizeof(buf), 5000);

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
    Serial.begin(115200);
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
    if (Serial.available())
    {
        char cmd = Serial.read();

        switch (cmd)
        {
        case 's':
            send_store_data();
            break;
        case 'c':
            send_clear_data();
            break;
        case 'v':
            query_version();
            break;
        case 'd':
            query_stored_data();
            break;
        }
    }
}
