/**
 * @file peripheral_example.ino
 * @brief Minimal CRUMBS slice for the triple board rig.
 *
 * Receives LED settings from the controller and replies with current state.
 */

#include <Arduino.h>
#include <Wire.h>
#include <CRUMBS.h>
#include <U8g2lib.h>
#include <SPI.h>

// ---------- Configuration ----------
const uint8_t kSliceI2cAddress = 0x09; // Change to 0x09 on the second peripheral

const uint8_t LED_GREEN = 4;
const uint8_t LED_YELLOW = 6;
const uint8_t LED_RED = 5;

const uint8_t OLED_ADDR = 0x3C;
const uint8_t OLED_SCL = 8;
const uint8_t OLED_SDA = 7;

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

CRUMBS crumbsSlice(false, kSliceI2cAddress);

struct LedChannel
{
    uint8_t pin;
    float ratio;
    unsigned long periodMs;
    bool state;
    unsigned long lastToggleMs;
};

LedChannel ledChannels[3] = {
    {LED_GREEN, 0.0f, 1000UL, false, 0UL},
    {LED_YELLOW, 0.0f, 1000UL, false, 0UL},
    {LED_RED, 0.0f, 1000UL, false, 0UL}};

struct MessageSummary
{
    bool valid = false;
    CRUMBSMessage message = {};
};

MessageSummary lastCommand;
MessageSummary lastResponse;

bool hasError = false;

unsigned long lastDisplayRefresh = 0;
const unsigned long kDisplayIntervalMs = 100;

// ---------- Helpers shared with companion files ----------
void drawDisplay();
void handleMessage(CRUMBSMessage &message);
void handleRequest();
void applyCommand(const CRUMBSMessage &message);
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

    crumbsSlice.begin();
    crumbsSlice.onReceive(handleMessage);
    crumbsSlice.onRequest(handleRequest);

    setOk();
    drawDisplay();

    Serial.print(F("CRUMBS slice ready at 0x"));
    Serial.println(kSliceI2cAddress, HEX);
}

// =======================================================
void loop()
{
    const unsigned long now = millis();
    updateChannels(now);

    if (now - lastDisplayRefresh >= kDisplayIntervalMs)
    {
        lastDisplayRefresh = now;
        drawDisplay();
    }
}

// =======================================================
void updateChannels(unsigned long now)
{
    for (LedChannel &channel : ledChannels)
    {
        const float ratio = constrain(channel.ratio, 0.0f, 1.0f);
        const unsigned long period = channel.periodMs == 0 ? 1UL : channel.periodMs;

        if (ratio <= 0.0f)
        {
            channel.state = false;
            digitalWrite(channel.pin, LOW);
            continue;
        }

        if (ratio >= 1.0f)
        {
            channel.state = true;
            digitalWrite(channel.pin, HIGH);
            continue;
        }

        unsigned long highDuration = static_cast<unsigned long>(period * ratio);
        if (highDuration == 0)
        {
            highDuration = 1;
        }
        unsigned long lowDuration = (period > highDuration) ? (period - highDuration) : 1UL;
        const unsigned long target = channel.state ? highDuration : lowDuration;

        if (now - channel.lastToggleMs >= target)
        {
            channel.state = !channel.state;
            channel.lastToggleMs = now;
            digitalWrite(channel.pin, channel.state ? HIGH : LOW);
        }
    }
}

void applyCommand(const CRUMBSMessage &message)
{
    const unsigned long now = millis();
    const unsigned long defaultPeriod = 1000UL;

    unsigned long requestedPeriod = defaultPeriod;
    if (message.data[3] > 0.0f)
    {
        requestedPeriod = static_cast<unsigned long>(message.data[3]);
    }
    if (requestedPeriod == 0)
    {
        requestedPeriod = defaultPeriod;
    }

    for (size_t i = 0; i < 3; ++i)
    {
        LedChannel &channel = ledChannels[i];
        channel.ratio = constrain(message.data[i], 0.0f, 1.0f);
        channel.periodMs = requestedPeriod;
        channel.lastToggleMs = now;

        if (channel.ratio <= 0.0f)
        {
            channel.state = false;
            digitalWrite(channel.pin, LOW);
        }
        else
        {
            channel.state = true;
            digitalWrite(channel.pin, HIGH);
        }
    }
}

void setOk()
{
    hasError = false;
}

void setError()
{
    hasError = true;
}
