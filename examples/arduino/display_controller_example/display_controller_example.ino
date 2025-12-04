/**
 * @file display_controller_example.ino
 * @brief CRUMBS controller with OLED display for the triple board rig.
 *
 * Serial commands (newline terminated, space-separated):
 *   SEND <addr> <type_id> <cmd> [byte0 byte1 ...]
 *   REQUEST <addr>
 *
 * Bytes can be decimal or hex (0x..).
 */

#include <Arduino.h>

#define CRUMBS_DEBUG
#include "crumbs_arduino.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>

// ---------- OLED (software I2C) ----------
const uint8_t OLED_ADDR = 0x3C; // SSD1306 128x64 default
const uint8_t OLED_SCL = 8;     // software I2C SCL
const uint8_t OLED_SDA = 7;     // software I2C SDA

// ---------- Hardware configuration ----------
const uint8_t LED_GREEN = 4;
const uint8_t LED_YELLOW = 6;
const uint8_t LED_RED = 5;

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// ---------- CRUMBS controller (C API) ----------
crumbs_context_t crumbs_ctx;

// ---------- Runtime state exposed to helper files ----------
struct LastExchange
{
    bool hasData = false;
    bool wasRx = false; // false = last action was SEND
    uint8_t address = 0;
    bool success = false; // true when wire transaction succeeded
    crumbs_message_t message = {};
} lastExchange;

bool hasError = false;
unsigned long activityPulseUntil = 0;
const unsigned long kActivityPulseMs = 120;

// ---------- Internal helpers (implemented below / other .ino files) ----------
void handleSerial();
void drawDisplay();

void sendCrumbs(uint8_t address, crumbs_message_t &message);
void requestCrumbs(uint8_t address);
void rememberExchange(bool wasRx, uint8_t address, const crumbs_message_t &message, bool success);
void pulseActivity();
void refreshIndicators();
void setOk();
void setError();

// Display refresh cadence
unsigned long lastDisplayRefresh = 0;
const unsigned long kDisplayIntervalMs = 100;

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

    // Initialize CRUMBS controller using Arduino HAL
    crumbs_arduino_init_controller(&crumbs_ctx);
    setOk();

    Serial.println(F("CRUMBS Controller ready."));
    Serial.println(F("Commands:"));
    Serial.println(F("  SEND <addr> <type> <cmd> [byte0 byte1 ...]"));
    Serial.println(F("  REQUEST <addr>"));
    Serial.println(F("Examples:"));
    Serial.println(F("  SEND 8 1 1 0x00 0x00 0x80 0x3F"));
    Serial.println(F("  REQUEST 0x08"));
}

// =======================================================
void loop()
{
    handleSerial();
    refreshIndicators();

    const unsigned long now = millis();
    if (now - lastDisplayRefresh >= kDisplayIntervalMs)
    {
        lastDisplayRefresh = now;
        drawDisplay();
    }
}

// =======================================================
void pulseActivity()
{
    activityPulseUntil = millis() + kActivityPulseMs;
}

void refreshIndicators()
{
    digitalWrite(LED_YELLOW, (millis() < activityPulseUntil) ? HIGH : LOW);
}

void setOk()
{
    hasError = false;
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
}

void setError()
{
    hasError = true;
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
}

void rememberExchange(bool wasRx, uint8_t address, const crumbs_message_t &message, bool success)
{
    lastExchange.hasData = true;
    lastExchange.wasRx = wasRx;
    lastExchange.address = address;
    lastExchange.success = success;
    lastExchange.message = message;
}

void sendCrumbs(uint8_t address, crumbs_message_t &message)
{
    uint8_t buffer[CRUMBS_MESSAGE_MAX_SIZE];
    size_t bytes = crumbs_encode_message(&message, buffer, sizeof(buffer));
    if (bytes == 0)
    {
        Serial.println(F("Encode failed (buffer too small or invalid data_len)."));
        rememberExchange(false, address, message, false);
        setError();
        return;
    }

    message.crc8 = buffer[bytes - 1];

    Wire.beginTransmission(address);
    size_t written = Wire.write(buffer, bytes);
    uint8_t err = Wire.endTransmission();

    if (err == 0 && written == bytes)
    {
        Serial.print(F("Sent to 0x"));
        Serial.print(address, HEX);
        Serial.print(F(" | data_len="));
        Serial.print(message.data_len);
        Serial.print(F(" bytes"));
        Serial.println();

        rememberExchange(false, address, message, true);
        pulseActivity();
        setOk();
    }
    else
    {
        Serial.print(F("I2C send failed. err="));
        Serial.print(err);
        Serial.print(F(" written="));
        Serial.println(written);

        rememberExchange(false, address, message, false);
        setError();
    }
}

void requestCrumbs(uint8_t address)
{
    /* Request up to CRUMBS_MESSAGE_MAX_SIZE bytes — cast to int for explicit overload */
    Wire.requestFrom((int)address, (int)CRUMBS_MESSAGE_MAX_SIZE);
    delay(5);

    uint8_t buffer[CRUMBS_MESSAGE_MAX_SIZE];
    size_t count = 0;
    while (Wire.available() && count < CRUMBS_MESSAGE_MAX_SIZE)
    {
        buffer[count++] = Wire.read();
    }

    if (count == 0)
    {
        Serial.println(F("No data received."));
        rememberExchange(true, address, (crumbs_message_t){}, false);
        setError();
        return;
    }

    crumbs_message_t message = {};
    if (crumbs_decode_message(buffer, count, &message, &crumbs_ctx) == 0)
    {
        Serial.print(F("Response from 0x"));
        Serial.print(address, HEX);
        Serial.print(F(" | data_len="));
        Serial.print(message.data_len);
        Serial.print(F(" bytes: "));
        for (int i = 0; i < message.data_len && i < 8; ++i)
        {
            Serial.print(F("0x"));
            if (message.data[i] < 0x10) Serial.print('0');
            Serial.print(message.data[i], HEX);
            Serial.print(' ');
        }
        Serial.println();

        rememberExchange(true, address, message, true);
        pulseActivity();
        setOk();
    }
    else
    {
        Serial.println(F("CRC or length check failed."));
        rememberExchange(true, address, message, false);
        setError();
    }
}
