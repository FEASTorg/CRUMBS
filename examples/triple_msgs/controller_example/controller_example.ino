/**
 * @file controller_example.ino
 * @brief CRUMBS Controller: sends messages and requests status from two peripherals.
 *
 * Serial commands:
 *   CSV send:      8,1,1,75.0,1.0,0.0,65.0,2.0,7.0,3.14
 *                  ^ addr,typeID,commandType,data0..data6
 *   Request read:  request=8
 */

#define CRUMBS_DEBUG
#include <CRUMBS.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <SPI.h>

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
CRUMBS crumbsController(true); // Master/controller

// ---------- Last message cache for display ----------
struct LastMsg
{
    bool isRx = false; // false: last TX, true: last RX
    uint8_t addr = 0;
    uint8_t typeID = 0;
    uint8_t commandType = 0;
    float data[CRUMBS_DATA_LENGTH] = {};
    uint8_t crc8 = 0;
    bool valid = false;
} lastMsg;

namespace
{
    uint32_t lastCrcReport = 0;
    bool lastCrcValid = true;

    void reportCrcStatus(const __FlashStringHelper *context)
    {
        const uint32_t current = crumbsController.getCrcErrorCount();
        const bool lastValid = crumbsController.isLastCrcValid();

        if (current != lastCrcReport || lastValid != lastCrcValid)
        {
            Serial.print(F("Controller: CRC status ["));
            Serial.print(context);
            Serial.print(F("] errors="));
            Serial.print(current);
            Serial.print(F(" lastValid="));
            Serial.println(lastValid ? F("true") : F("false"));
            lastCrcReport = current;
            lastCrcValid = lastValid;
        }
    }
}

// Prototypes (helpers are in .ino companions)
void handleSerialInput();
bool parseSerialInput(const String &, uint8_t &, CRUMBSMessage &);
void drawDisplay();
void setOk();
void setError();
void pulseActivity();
void refreshLeds();

// ================================
void setup()
{
    // LEDs
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

    // OLED
    u8g2.setI2CAddress(OLED_ADDR << 1);
    u8g2.begin();

    // CRUMBS/I2C
    crumbsController.begin();
    crumbsController.resetCrcErrorCount();
    Serial.println(F("Controller: CRC diagnostics reset."));

    setOk(); // system starts OK

    Serial.println(F("Controller ready."));
    Serial.println(F("CSV send: addr,typeID,commandType,data0..data6"));
    Serial.println(F("Request : request=<addr>  (e.g. request=8 or request=0x08)"));
}

void loop()
{
    handleSerialInput();
    refreshLeds(); // keep activity pulse timing non-blocking
}

// Utilities to update last message cache
static void cacheTx(uint8_t addr, const CRUMBSMessage &m)
{
    lastMsg.isRx = false;
    lastMsg.addr = addr;
    lastMsg.typeID = m.typeID;
    lastMsg.commandType = m.commandType;
    for (size_t i = 0; i < CRUMBS_DATA_LENGTH; i++)
        lastMsg.data[i] = m.data[i];
    lastMsg.crc8 = m.crc8;
    lastMsg.valid = true;
    drawDisplay();
}

static void cacheRx(uint8_t addr, const CRUMBSMessage &m)
{
    lastMsg.isRx = true;
    lastMsg.addr = addr;
    lastMsg.typeID = m.typeID;
    lastMsg.commandType = m.commandType;
    for (size_t i = 0; i < CRUMBS_DATA_LENGTH; i++)
        lastMsg.data[i] = m.data[i];
    lastMsg.crc8 = m.crc8;
    lastMsg.valid = true;
    drawDisplay();
}

// I2C send wrapper with LED/error handling
static void sendCrumbs(uint8_t addr, const CRUMBSMessage &m)
{
    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t n = crumbsController.encodeMessage(m, buffer, sizeof(buffer));
    if (n == 0)
    {
        Serial.println(F("Controller: encode failed."));
        setError();
        return;
    }
    Wire.beginTransmission(addr);
    size_t written = Wire.write(buffer, n);
    uint8_t err = Wire.endTransmission();
    Serial.print(F("Controller: TX bytes="));
    Serial.print(written);
    Serial.print(F(" status="));
    Serial.println(err);

    if (err == 0)
    {
        pulseActivity();
        cacheTx(addr, m);
        setOk();
    }
    else
    {
        setError();
    }

    reportCrcStatus(F("send"));
}

// I2C request wrapper with LED/error handling
static bool requestCrumbs(uint8_t addr, CRUMBSMessage &out)
{
    Wire.requestFrom(addr, (uint8_t)CRUMBS_MESSAGE_SIZE);
    delay(5); // small settle
    uint8_t buf[CRUMBS_MESSAGE_SIZE];
    int i = 0;
    while (Wire.available() && i < CRUMBS_MESSAGE_SIZE)
        buf[i++] = Wire.read();

    Serial.print(F("Controller: RX bytes="));
    Serial.println(i);
    if (i == 0)
    {
        setError();
        return false;
    }

    if (crumbsController.decodeMessage(buf, i, out))
    {
        pulseActivity();
        cacheRx(addr, out);
        setOk();
        reportCrcStatus(F("request"));
        return true;
    }
    Serial.println(F("Controller: decode failed."));
    setError();
    reportCrcStatus(F("request"));
    return false;
}
