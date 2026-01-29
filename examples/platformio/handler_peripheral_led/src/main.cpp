/**
 * @file main.cpp
 * @brief LED Peripheral using CRUMBS command handlers.
 *
 * Hardware:
 * - Arduino Nano (or compatible)
 * - 4 LEDs on pins D4, D5, D6, D7
 * - I2C address: 0x08
 *
 * Commands handled:
 * - LED_CMD_SET_ALL: Set all LEDs using bitmask
 * - LED_CMD_SET_ONE: Set single LED on/off
 * - LED_CMD_BLINK: Blink all LEDs
 * - LED_CMD_GET_STATE: Return current state via I2C read
 */

#include <Arduino.h>
#include <crumbs.h>
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

#include "led_commands.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define I2C_ADDRESS 0x08

static const uint8_t LED_PINS[] = {4, 5, 6, 7};
static const uint8_t NUM_LEDS = 4;

/* ============================================================================
 * State
 * ============================================================================ */

static uint8_t led_state = 0; /* Bitmask of current LED states */
static crumbs_context_t ctx;

/* ============================================================================
 * Hardware Control
 * ============================================================================ */

/**
 * @brief Apply LED state bitmask to hardware.
 */
static void apply_state(uint8_t state)
{
    led_state = state;
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        digitalWrite(LED_PINS[i], (state >> i) & 1 ? HIGH : LOW);
    }
}

/* ============================================================================
 * Command Handlers
 * ============================================================================ */

/**
 * @brief Handler for LED_CMD_SET_ALL.
 */
static void handle_set_all(crumbs_context_t *ctx, uint8_t cmd,
                           const uint8_t *data, uint8_t len, void *user)
{
    (void)ctx;
    (void)cmd;
    (void)user;

    uint8_t bitmask;
    if (crumbs_msg_read_u8(data, len, 0, &bitmask) == 0)
    {
        apply_state(bitmask & 0x0F); /* Mask to 4 LEDs */

        Serial.print(F("SET_ALL: 0b"));
        Serial.println(led_state, BIN);
    }
}

/**
 * @brief Handler for LED_CMD_SET_ONE.
 */
static void handle_set_one(crumbs_context_t *ctx, uint8_t cmd,
                           const uint8_t *data, uint8_t len, void *user)
{
    (void)ctx;
    (void)cmd;
    (void)user;

    uint8_t index, state;
    if (crumbs_msg_read_u8(data, len, 0, &index) == 0 &&
        crumbs_msg_read_u8(data, len, 1, &state) == 0)
    {

        if (index < NUM_LEDS)
        {
            if (state)
            {
                led_state |= (1 << index);
            }
            else
            {
                led_state &= ~(1 << index);
            }
            apply_state(led_state);

            Serial.print(F("SET_ONE: LED"));
            Serial.print(index);
            Serial.print(F(" = "));
            Serial.println(state ? F("ON") : F("OFF"));
        }
    }
}

/**
 * @brief Handler for LED_CMD_BLINK.
 */
static void handle_blink(crumbs_context_t *ctx, uint8_t cmd,
                         const uint8_t *data, uint8_t len, void *user)
{
    (void)ctx;
    (void)cmd;
    (void)user;

    uint8_t count, delay_10ms;
    if (crumbs_msg_read_u8(data, len, 0, &count) == 0 &&
        crumbs_msg_read_u8(data, len, 1, &delay_10ms) == 0)
    {

        Serial.print(F("BLINK: count="));
        Serial.print(count);
        Serial.print(F(", delay="));
        Serial.print(delay_10ms * 10);
        Serial.println(F("ms"));

        uint8_t saved = led_state;
        for (uint8_t i = 0; i < count; i++)
        {
            apply_state(0x0F); /* All on */
            delay(delay_10ms * 10);
            apply_state(0x00); /* All off */
            delay(delay_10ms * 10);
        }
        apply_state(saved); /* Restore previous state */
    }
}

/**
 * @brief Request handler for I2C read (GET_STATE response).
 */
static void handle_request(struct crumbs_context_s *ctx, crumbs_message_t *reply)
{
    (void)ctx;

    crumbs_msg_init(reply, LED_TYPE_ID, LED_CMD_GET_STATE);
    crumbs_msg_add_u8(reply, led_state);

    Serial.print(F("GET_STATE: 0b"));
    Serial.println(led_state, BIN);
}

/* ============================================================================
 * Arduino Setup & Loop
 * ============================================================================ */

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }
    Serial.println(F("LED Peripheral - PlatformIO"));

    /* Initialize LED pins */
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }

    /* Verify struct size matches library */
    if (sizeof(ctx) != crumbs_context_size())
    {
        Serial.println(F("ERROR: Context size mismatch!"));
        Serial.print(F("Sketch: "));
        Serial.print(sizeof(ctx));
        Serial.print(F("  Library: "));
        Serial.println(crumbs_context_size());
        while (1)
        {
            delay(1000);
        }
    }

    /* Initialize CRUMBS peripheral */
    crumbs_arduino_init_peripheral(&ctx, I2C_ADDRESS);

    /* Set up request callback for I2C reads */
    crumbs_set_callbacks(&ctx, nullptr, handle_request, nullptr);

    /* Register command handlers */
    crumbs_register_handler(&ctx, LED_CMD_SET_ALL, handle_set_all, nullptr);
    crumbs_register_handler(&ctx, LED_CMD_SET_ONE, handle_set_one, nullptr);
    crumbs_register_handler(&ctx, LED_CMD_BLINK, handle_blink, nullptr);

    Serial.println(F("=== LED Peripheral Ready ==="));
    Serial.print(F("I2C Address: 0x"));
    Serial.println(I2C_ADDRESS, HEX);
    Serial.print(F("Max handlers: "));
    Serial.println(CRUMBS_MAX_HANDLERS);
    Serial.print(F("Context size: "));
    Serial.println(sizeof(ctx));

    Serial.print(F("LEDs on pins: "));
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        Serial.print(LED_PINS[i]);
        if (i < NUM_LEDS - 1)
            Serial.print(F(", "));
    }
    Serial.println();
}

void loop()
{
    /* I2C processing happens via callbacks */
}
