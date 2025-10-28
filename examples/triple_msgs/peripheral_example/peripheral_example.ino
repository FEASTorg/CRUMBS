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
#define SLICE_I2C_ADDRESS 0x08 // Change to 0x09 on the second peripheral

// ---------- OLED (software I2C) ----------
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t OLED_SCL = 8; // D8
constexpr uint8_t OLED_SDA = 7; // D7
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// ---------- LEDs ----------
constexpr uint8_t LED_GREEN = 4;
constexpr uint8_t LED_YELLOW = 6;
constexpr uint8_t LED_RED = 5;

// Activity pulse timing
volatile unsigned long yellowPulseUntil = 0;
constexpr unsigned long YELLOW_PULSE_MS = 120;

// ---------- CRUMBS ----------
CRUMBS crumbsSlice(false, SLICE_I2C_ADDRESS);

// ---------- State controlled by incoming CRUMBS data ----------
struct LedChan
{
    float ratio;          // 0.0..1.0 duty
    unsigned long period; // ms
    bool state;
    uint8_t pin;
};
LedChan chans[3] = {
    {0.5f, 2000UL, false, LED_GREEN},  // We'll drive green as "OK" unless error overrides
    {0.0f, 1000UL, false, LED_YELLOW}, // Yellow as activity pulse + optional CRUMBS control
    {0.0f, 1000UL, false, LED_RED}     // Red indicates error if errorFlags!=0; can be CRUMBS driven
};

unsigned long lastBlinkCheck = 0;

// Last received message (for OLED)
struct LastMsg
{
    uint8_t typeID = 0;
    uint8_t commandType = 0;
    float data[6] = {0, 0, 0, 0, 0, 0};
    uint8_t errorFlags = 0;
    bool valid = false;
} lastRx;

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
    crumbsSlice.onReceive(handleMessage);
    crumbsSlice.onRequest(handleRequest);

    setOk();
    drawDisplay();
    Serial.print(F("Peripheral ready at 0x"));
    Serial.println(SLICE_I2C_ADDRESS, HEX);
}

void loop()
{
    refreshLeds();
    serviceBlinkLogic(); // optional: CRUMBS-driven blink patterns
}
