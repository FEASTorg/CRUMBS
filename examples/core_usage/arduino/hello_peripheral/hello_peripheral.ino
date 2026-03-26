/**
 * @file hello_peripheral.ino
 * @brief Minimal CRUMBS peripheral example - Start here!
 */

#include <crumbs.h>
#include <crumbs_arduino.h>
#include "config.h"

crumbs_context_t ctx;
uint8_t counter = 0;
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

/* NOTE: hello_peripheral uses on_message for brevity (one callback, no handler table).
 * For a real family peripheral, use crumbs_register_handler() for each SET opcode instead.
 * See examples/families_usage/ for the correct pattern. */
void on_message(crumbs_context_t *ctx, const crumbs_message_t *msg)
{
    Serial.print("RX cmd=");
    Serial.println(msg->opcode);
    counter++;
}

void on_request(crumbs_context_t *ctx, crumbs_message_t *reply)
{
    reply->type_id = DEVICE_TYPE_ID;
    reply->opcode = DEFAULT_REPLY_OPCODE;
    reply->data_len = 1;
    reply->data[0] = counter;
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    /* Guard: catches CRUMBS_MAX_HANDLERS mismatch between sketch and library.
     * If this fires, define CRUMBS_MAX_HANDLERS in build_flags (platformio.ini
     * or boards.txt), not in the sketch itself. See crumbs.h for details. */
    if (sizeof(crumbs_context_t) != crumbs_context_size())
    {
        Serial.println(F("FATAL: CRUMBS_MAX_HANDLERS mismatch! See crumbs.h."));
        while (1);
    }

    crumbs_arduino_init_peripheral(&ctx, DEVICE_ADDR);
    crumbs_set_callbacks(&ctx, on_message, on_request, NULL);
    Serial.print("Hello peripheral at 0x");
    Serial.println(DEVICE_ADDR, HEX);
}

void loop()
{
    heartbeat_tick();
    delay(1);
}
