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

// ---------- Status LED state ----------
// Green = solid when OK
// Red = blink when ERROR
// Yellow = activity pulse on CRUMBS message RX
unsigned long errorBlinkLast = 0;
const unsigned long ERROR_BLINK_MS = 250;
bool errorBlinkState = false;
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
void refreshLeds();
void pulseActivity();
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
}
