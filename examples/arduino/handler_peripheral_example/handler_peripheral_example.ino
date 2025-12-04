/**
 * @file handler_peripheral_example.ino
 * @brief CRUMBS peripheral example demonstrating per-command handler dispatch.
 *
 * This example shows how to use crumbs_register_handler() to register
 * dedicated functions for specific command types, as an alternative to
 * using a switch statement in a single on_message callback.
 *
 * Commands demonstrated:
 *   0x01: LED_ON  - Turn on the built-in LED
 *   0x02: LED_OFF - Turn off the built-in LED
 *   0x03: BLINK   - Blink LED (data[0] = count, data[1] = delay_ms/10)
 *   0x10: ECHO    - Echo payload back in next request
 */

#include <crumbs.h>
#include <crumbs_arduino.h>

#define SLICE_I2C_ADDRESS 0x08
#define LED_PIN LED_BUILTIN

// Command type constants
#define CMD_LED_ON   0x01
#define CMD_LED_OFF  0x02
#define CMD_BLINK    0x03
#define CMD_ECHO     0x10

static crumbs_context_t ctx;

// Echo buffer - stores last ECHO payload for reply
static uint8_t echoBuffer[CRUMBS_MAX_PAYLOAD];
static uint8_t echoLen = 0;

/* --------------------------------------------------------------------------
 * Command Handlers
 * -------------------------------------------------------------------------- */

/**
 * @brief Handler for CMD_LED_ON (0x01) - Turn LED on.
 */
void handleLedOn(crumbs_context_t *ctx,
                 uint8_t cmd,
                 const uint8_t *data,
                 uint8_t len,
                 void *user)
{
    (void)ctx; (void)cmd; (void)data; (void)len; (void)user;

    digitalWrite(LED_PIN, HIGH);
    Serial.println(F("LED ON"));
}

/**
 * @brief Handler for CMD_LED_OFF (0x02) - Turn LED off.
 */
void handleLedOff(crumbs_context_t *ctx,
                  uint8_t cmd,
                  const uint8_t *data,
                  uint8_t len,
                  void *user)
{
    (void)ctx; (void)cmd; (void)data; (void)len; (void)user;

    digitalWrite(LED_PIN, LOW);
    Serial.println(F("LED OFF"));
}

/**
 * @brief Handler for CMD_BLINK (0x03) - Blink LED.
 *
 * Payload: [count, delay_10ms]
 *   count     - number of blinks (1-255)
 *   delay_10ms - delay in 10ms units (e.g., 10 = 100ms)
 */
void handleBlink(crumbs_context_t *ctx,
                 uint8_t cmd,
                 const uint8_t *data,
                 uint8_t len,
                 void *user)
{
    (void)ctx; (void)cmd; (void)user;

    if (len < 2)
    {
        Serial.println(F("BLINK: need 2 bytes"));
        return;
    }

    uint8_t count = data[0];
    uint8_t delayMs = data[1] * 10;

    Serial.print(F("BLINK: count="));
    Serial.print(count);
    Serial.print(F(" delay="));
    Serial.println(delayMs);

    for (uint8_t i = 0; i < count; i++)
    {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}

/**
 * @brief Handler for CMD_ECHO (0x10) - Store payload for echo reply.
 *
 * The payload is stored and will be returned on the next I2C request.
 */
void handleEcho(crumbs_context_t *ctx,
                uint8_t cmd,
                const uint8_t *data,
                uint8_t len,
                void *user)
{
    (void)ctx; (void)cmd; (void)user;

    echoLen = (len <= CRUMBS_MAX_PAYLOAD) ? len : CRUMBS_MAX_PAYLOAD;
    if (echoLen > 0)
    {
        memcpy(echoBuffer, data, echoLen);
    }

    Serial.print(F("ECHO: stored "));
    Serial.print(echoLen);
    Serial.println(F(" bytes"));
}

/* --------------------------------------------------------------------------
 * Optional: General message callback for logging
 * -------------------------------------------------------------------------- */

/**
 * @brief General on_message callback for logging all received messages.
 *
 * This runs BEFORE the per-command handler, so you can use it for
 * logging, statistics, or pre-processing.
 */
void onMessage(crumbs_context_t *ctx, const crumbs_message_t *msg)
{
    Serial.print(F("RX cmd=0x"));
    Serial.print(msg->command_type, HEX);
    Serial.print(F(" len="));
    Serial.println(msg->data_len);
}

/**
 * @brief Request callback - send echo buffer as reply.
 */
void onRequest(crumbs_context_t *ctx, crumbs_message_t *reply)
{
    reply->type_id = 1;
    reply->command_type = CMD_ECHO;
    reply->data_len = echoLen;
    if (echoLen > 0)
    {
        memcpy(reply->data, echoBuffer, echoLen);
    }
}

/* --------------------------------------------------------------------------
 * Setup and Loop
 * -------------------------------------------------------------------------- */

void setup()
{
    Serial.begin(115200);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // Initialize CRUMBS peripheral
    crumbs_arduino_init_peripheral(&ctx, SLICE_I2C_ADDRESS);

    // Install general callbacks (optional - handlers work without these)
    crumbs_set_callbacks(&ctx, onMessage, onRequest, nullptr);

    // Register per-command handlers
    crumbs_register_handler(&ctx, CMD_LED_ON,  handleLedOn,  nullptr);
    crumbs_register_handler(&ctx, CMD_LED_OFF, handleLedOff, nullptr);
    crumbs_register_handler(&ctx, CMD_BLINK,   handleBlink,  nullptr);
    crumbs_register_handler(&ctx, CMD_ECHO,    handleEcho,   nullptr);

    Serial.println(F("Handler Peripheral Ready"));
    Serial.print(F("I2C Address: 0x"));
    Serial.println(SLICE_I2C_ADDRESS, HEX);
    Serial.println(F("Commands: 0x01=ON, 0x02=OFF, 0x03=BLINK, 0x10=ECHO"));
}

void loop()
{
    // All processing is event-driven via handlers
}
