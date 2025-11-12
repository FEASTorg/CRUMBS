/**
 * @file peripheral_example.ino
 * @brief CRUMBS Peripheral: receives messages from controller and answers status requests.
 *
 * Define a unique I2C address for each peripheral board before flashing.
 */

#define CRUMBS_DEBUG
#include <CRUMBS.h>
#include <Wire.h>
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

// ---------- CRUMBS ----------
CRUMBS crumbsSlice(false, kSliceI2cAddress);

// ---------- State controlled by incoming CRUMBS data ----------
struct LedChan
{
    float ratio;               // 0.0..1.0 duty
    unsigned long period;      // ms
    bool state;
    uint8_t pin;
    unsigned long lastToggle;  // ms timestamp of last state change
};
LedChan chans[3] = {
    {0.5f, 2000UL, false, LED_GREEN, 0UL},  // We'll drive green as "OK" unless another mode overrides
    {0.0f, 1000UL, false, LED_YELLOW, 0UL}, // Yellow as activity pulse + optional CRUMBS control
    {0.0f, 1000UL, false, LED_RED, 0UL}     // Red can be CRUMBS driven for fault indication
};
// Last received message (for OLED)
struct LastMsg
{
    uint8_t typeID = 0;
    uint8_t commandType = 0;
    float data[CRUMBS_DATA_LENGTH] = {};
    uint8_t crc8 = 0;
    bool valid = false;
} lastRx;

namespace
{
    uint32_t lastCrcReport = 0;
    bool lastCrcValid = true;
}

void reportCrcStatus(const __FlashStringHelper *context)
{
    const uint32_t current = crumbsSlice.getCrcErrorCount();
    const bool lastValid = crumbsSlice.isLastCrcValid();

    if (current != lastCrcReport || lastValid != lastCrcValid)
    {
        Serial.print(F("Slice: CRC status ["));
        Serial.print(context);
        Serial.print(F("] errors="));
        Serial.print(current);
        Serial.print(F(" lastValid="));
        Serial.println(lastValid ? F("true") : F("false"));
        lastCrcReport = current;
        lastCrcValid = lastValid;
    }
}

// Prototypes (helpers in companions)
void handleMessage(CRUMBSMessage &m);
void handleRequest();
void drawDisplay();
void setOk();
void setError();
void pulseActivity();
void refreshLeds();
void applyDataToChannels(const CRUMBSMessage &m);
void serviceBlinkLogic();

// ================================
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
    crumbsSlice.resetCrcErrorCount();
    Serial.println(F("Slice: CRC diagnostics reset."));
    crumbsSlice.onReceive(handleMessage);
    crumbsSlice.onRequest(handleRequest);

    setOk();
    drawDisplay();
    Serial.print(F("Peripheral ready at 0x"));
    Serial.println(kSliceI2cAddress, HEX);
    reportCrcStatus(F("startup"));
}

void loop()
{
    refreshLeds();
    serviceBlinkLogic(); // optional: CRUMBS-driven blink patterns
    reportCrcStatus(F("loop"));
}

void applyDataToChannels(const CRUMBSMessage &m)
{
    const unsigned long now = millis();
    const unsigned long kDefaultPeriod = 1000UL;
    const unsigned long kMinPeriod = 100UL;
    const unsigned long kMaxPeriod = 10000UL;

    unsigned long requestedPeriod = kDefaultPeriod;
    if (m.data[3] > 0.0f)
    {
        requestedPeriod = static_cast<unsigned long>(m.data[3] * 1000.0f);
    }
    requestedPeriod = constrain(requestedPeriod, kMinPeriod, kMaxPeriod);

    for (size_t i = 0; i < 3; i++)
    {
        LedChan &chan = chans[i];
        const float raw = (i < CRUMBS_DATA_LENGTH) ? m.data[i] : 0.0f;
        const float ratio = constrain(raw, 0.0f, 1.0f);

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
