/**
 * @file peripheral_example.ino
 * @brief Minimal CRUMBS slice for the triple board rig.
 *
 * Receives LED settings from the controller and replies with current state.
 */

#include <Arduino.h>
#include <Wire.h>
#include "crumbs_arduino.h"
#include <U8g2lib.h>
#include <SPI.h>

// ---------- CONFIG: set each board's address ----------
const uint8_t kSliceI2cAddress = 0x08; // Change to 0x09 on the second peripheral

// ---------- OLED (software I2C) ----------
const uint8_t OLED_ADDR = 0x3C;
const uint8_t OLED_SCL = 8; // D8
const uint8_t OLED_SDA = 7; // D7
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// ---------- LEDs ----------
const uint8_t LED_GREEN = 4;
const uint8_t LED_YELLOW = 6;
const uint8_t LED_RED = 5;

// Activity pulse timing
volatile unsigned long yellowPulseUntil = 0;
const unsigned long YELLOW_PULSE_MS = 120;

// ---------- CRUMBS (C API) ----------
crumbs_context_t crumbs_ctx;

// ---------- State controlled by incoming CRUMBS data ----------
struct LedChan
{
    float ratio;          // 0.0..1.0 duty
    unsigned long period; // ms
    bool state;
    uint8_t pin;
    unsigned long lastToggle; // ms timestamp of last state change
};
LedChan chans[3] = {
    {0.5f, 2000UL, false, LED_GREEN, 0UL},  // We'll drive green as "OK" unless another mode overrides
    {0.0f, 1000UL, false, LED_YELLOW, 0UL}, // Yellow as activity pulse + optional CRUMBS control
    {0.0f, 1000UL, false, LED_RED, 0UL}     // Red can be CRUMBS driven for fault indication
};
// Last received/response messages (for the display)
crumbs_message_t lastRxMessage = {};
bool lastRxValid = false;

crumbs_message_t lastResponseMessage = {};
bool lastResponseValid = false;

bool hasError = false;

unsigned long lastDisplayRefresh = 0;
const unsigned long kDisplayIntervalMs = 100;

// ---------- Helpers shared with companion files ----------
void drawDisplay();
void handleMessage(crumbs_context_t *ctx, const crumbs_message_t *message);
void handleRequest(crumbs_context_t *ctx, crumbs_message_t *response);
void applyCommand(const crumbs_message_t &message);
void updateChannels(unsigned long now);
void setOk();
void setError();

// =======================================================
void setup()
{
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);

    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }

    u8g2.setI2CAddress(OLED_ADDR << 1);
    u8g2.begin();

    // Initialize peripheral and register callbacks
    crumbs_arduino_init_peripheral(&crumbs_ctx, kSliceI2cAddress);
    crumbs_set_callbacks(&crumbs_ctx, handleMessage, handleRequest, NULL);

    setOk();
    drawDisplay();
    Serial.print(F("Peripheral ready at 0x"));
    Serial.println(kSliceI2cAddress, HEX);
}

// =======================================================
void loop()
{
    refreshLeds();
    serviceBlinkLogic(); // optional: CRUMBS-driven blink patterns
}

void applyDataToChannels(const crumbs_message_t &m)
{
    const unsigned long now = millis();
    const unsigned long kDefaultPeriod = 1000UL;
    const unsigned long kMinPeriod = 100UL;
    const unsigned long kMaxPeriod = 10000UL;

    // Parse payload bytes into ratios and period
    // Expected payload format: 4 floats (16 bytes): [green_ratio, yellow_ratio, red_ratio, period_s]
    // Or raw bytes that user interprets as needed
    
    float ratios[3] = {0.5f, 0.0f, 0.0f};
    unsigned long requestedPeriod = kDefaultPeriod;

    if (m.data_len >= 12) // At least 3 floats
    {
        memcpy(&ratios[0], &m.data[0], sizeof(float));
        memcpy(&ratios[1], &m.data[4], sizeof(float));
        memcpy(&ratios[2], &m.data[8], sizeof(float));
    }
    if (m.data_len >= 16) // 4th float for period
    {
        float period_s = 0.0f;
        memcpy(&period_s, &m.data[12], sizeof(float));
        if (period_s > 0.0f)
        {
            requestedPeriod = static_cast<unsigned long>(period_s * 1000.0f);
        }
    }
    requestedPeriod = constrain(requestedPeriod, kMinPeriod, kMaxPeriod);

    for (size_t i = 0; i < 3; i++)
    {
        LedChan &chan = chans[i];
        const float ratio = constrain(ratios[i], 0.0f, 1.0f);

        chan.ratio = ratio;
        chan.period = requestedPeriod;
        chan.lastToggle = now;

        if (ratio <= 0.0f)
        {
            chan.state = false;
            digitalWrite(chan.pin, LOW);
        }
        else if (ratio >= 1.0f)
        {
            chan.state = true;
            digitalWrite(chan.pin, HIGH);
        }
        else
        {
            chan.state = true; // restart cycle in high phase
            digitalWrite(chan.pin, HIGH);
        }
    }
}

void serviceBlinkLogic()
{
    const unsigned long now = millis();

    for (size_t i = 0; i < 3; i++)
    {
        LedChan &chan = chans[i];

        if (chan.ratio <= 0.0f)
        {
            continue; // forced low
        }
        if (chan.ratio >= 1.0f)
        {
            continue; // forced high
        }

        const unsigned long period = (chan.period < 2UL) ? 2UL : chan.period;
        unsigned long highDuration = static_cast<unsigned long>(period * chan.ratio);
        if (highDuration == 0)
        {
            highDuration = 1;
        }
        unsigned long lowDuration = (period > highDuration) ? (period - highDuration) : 1UL;

        const unsigned long targetDuration = chan.state ? highDuration : lowDuration;
        const unsigned long elapsed = now - chan.lastToggle;

        if (elapsed >= targetDuration)
        {
            chan.state = !chan.state;
            chan.lastToggle = now;
            digitalWrite(chan.pin, chan.state ? HIGH : LOW);
        }
    }
}
