/**
 * @file hello_controller.ino
 * @brief Minimal CRUMBS controller example - Pair with hello_peripheral
 */

#include <crumbs.h>
#include <crumbs_arduino.h>

crumbs_context_t ctx;

void send_hello()
{
    crumbs_message_t msg;
    msg.type_id = 1;
    msg.opcode = 1;
    msg.data_len = 2;
    msg.data[0] = 0xAA;
    msg.data[1] = 0xBB;

    crumbs_controller_send(&ctx, 0x08, &msg, crumbs_arduino_wire_write, NULL);
    Serial.println("Sent!");
}

void request_data()
{
    // Step 1: Send SET_REPLY to tell peripheral what we want
    crumbs_message_t query;
    query.type_id = 1;
    query.opcode = 0xFE; // SET_REPLY command
    query.data_len = 1;
    query.data[0] = 0x00; // Request opcode 0 (version/default)
    crumbs_controller_send(&ctx, 0x08, &query, crumbs_arduino_wire_write, NULL);

    // Step 2: Read the reply
    delay(10);
    uint8_t buf[32];
    int n = crumbs_arduino_read(NULL, 0x08, buf, sizeof(buf), 5000);

    if (n > 0)
    {
        Serial.print("Received ");
        Serial.print(n);
        Serial.println(" bytes");
    }
}

void setup()
{
    Serial.begin(115200);
    crumbs_arduino_init_controller(&ctx);
    Serial.println("Commands: s=send, r=request");
}

void loop()
{
    if (Serial.available())
    {
        char c = Serial.read();
        if (c == 's')
            send_hello();
        if (c == 'r')
            request_data();
    }
}
