// Arduino Nano + ELEGOO 0.96" SSD1306 OLED on software I2C (SCL=D8, SDA=D7)
// Non-blocking LED controller with serial commands and on-screen status
// Libraries (Library Manager): U8g2, SPI, Wire

#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

// ---------- OLED (software I2C) ----------
constexpr uint8_t OLED_ADDR_7BIT = 0x3C;
constexpr uint8_t OLED_SCL_PIN = 8; // D8
constexpr uint8_t OLED_SDA_PIN = 7; // D7

// Page-buffered constructor to fit Nano RAM
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(
    U8G2_R0, // rotation
    /* clock=*/OLED_SCL_PIN,
    /* data=*/OLED_SDA_PIN,
    /* reset=*/U8X8_PIN_NONE);

// ---------- LEDs ----------
struct Led
{
    uint8_t pin;
    float activeRatio;    // 0.0–1.0
    unsigned long period; // ms
    bool state;
};

Led leds[3] = {
    {4, 0.5f, 2000UL, false}, // Green
    {6, 0.5f, 2000UL, false}, // Yellow
    {5, 0.5f, 2000UL, false}  // Red
};

// ---------- Timers ----------
unsigned long lastReport = 0;
unsigned long lastDisplay = 0;
const unsigned long reportIntervalMs = 1000; // serial status
const unsigned long displayIntervalMs = 100; // ~10 FPS UI

// ---------- Forward decl ----------
void handleSerial();
void updateLed(const String &color, long period, float ratio);
void drawDisplay();

// =======================================================
void setup()
{
    Serial.begin(115200);

    // LEDs
    for (auto &led : leds)
    {
        pinMode(led.pin, OUTPUT);
        digitalWrite(led.pin, LOW);
    }

    // OLED
    // U8g2 expects 8-bit address; shift 7-bit by 1
    u8g2.setI2CAddress(OLED_ADDR_7BIT << 1);
    u8g2.begin();

    // Initial screen
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(0, 10, "Nano + SSD1306");
        u8g2.drawStr(0, 22, "Soft I2C D7/D8 OK");
        u8g2.drawStr(0, 34, "Type:");
        u8g2.drawStr(0, 46, "SET <color> <period> <ratio>");
        u8g2.drawStr(0, 58, "Ex: SET green 1000 0.25");
    } while (u8g2.nextPage());

    Serial.println(F("Ready. Commands:"));
    Serial.println(F("SET <color> <period_ms> <ratio_0to1>"));
    Serial.println(F("Example: SET green 1000 0.25"));
}

// =======================================================
void loop()
{
    const unsigned long now = millis();

    // Non-blocking serial
    handleSerial();

    // Non-blocking LED updates
    for (auto &led : leds)
    {
        // Compute position within current period
        const unsigned long elapsed = now % led.period;
        const unsigned long activeTime = (unsigned long)(led.period * led.activeRatio);
        const bool newState = (elapsed < activeTime);
        if (newState != led.state)
        {
            led.state = newState;
            digitalWrite(led.pin, newState ? HIGH : LOW);
        }
    }

    // Periodic serial status
    if (now - lastReport >= reportIntervalMs)
    {
        lastReport = now;
        Serial.print(F("Status | G:"));
        Serial.print(leds[0].activeRatio, 3);
        Serial.print(F(" Y:"));
        Serial.print(leds[1].activeRatio, 3);
        Serial.print(F(" R:"));
        Serial.print(leds[2].activeRatio, 3);
        Serial.print(F(" Period(ms):"));
        Serial.println(leds[0].period);
    }

    // Periodic OLED update
    if (now - lastDisplay >= displayIntervalMs)
    {
        lastDisplay = now;
        drawDisplay();
    }
}

// =======================================================
void handleSerial()
{
    if (!Serial.available())
        return;

    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (!cmd.startsWith("SET"))
        return;

    // Parse: SET <color> <period_ms> <ratio>
    int firstSpace = cmd.indexOf(' ');
    int secondSpace = cmd.indexOf(' ', firstSpace + 1);
    int thirdSpace = cmd.indexOf(' ', secondSpace + 1);
    if (firstSpace < 0 || secondSpace < 0 || thirdSpace < 0)
    {
        Serial.println(F("Invalid format. Use: SET <color> <period_ms> <ratio>"));
        return;
    }

    String color = cmd.substring(firstSpace + 1, secondSpace);
    String periodStr = cmd.substring(secondSpace + 1, thirdSpace);
    String ratioStr = cmd.substring(thirdSpace + 1);

    long period = periodStr.toInt();
    float ratio = ratioStr.toFloat();

    updateLed(color, period, ratio);
}

// =======================================================
void updateLed(const String &color, long period, float ratio)
{
    int idx = -1;
    if (color.equalsIgnoreCase("green"))
        idx = 0;
    else if (color.equalsIgnoreCase("yellow"))
        idx = 1;
    else if (color.equalsIgnoreCase("red"))
        idx = 2;

    if (idx >= 0 && ratio >= 0.0f && ratio <= 1.0f && period > 0)
    {
        leds[idx].period = (unsigned long)period;
        leds[idx].activeRatio = ratio;

        Serial.print(F("Updated "));
        Serial.print(color);
        Serial.print(F(": period="));
        Serial.print(period);
        Serial.print(F("ms ratio="));
        Serial.println(ratio, 3);
    }
    else
    {
        Serial.println(F("Invalid parameters."));
    }
}

// =======================================================
// Simple UI: shows period, duty ratio bars, and live ON/OFF state
void drawDisplay()
{
    const uint8_t x0 = 0;
    const uint8_t headingY = 0;
    const uint8_t headingH = 12; // top region (yellow)
    const uint8_t bodyY = headingY + headingH;
    const uint8_t lineH = 14;
    const uint8_t barW = 76;
    const uint8_t barH = 8;

    u8g2.firstPage();
    do
    {
        // Heading region (yellow) — use larger font
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(x0, headingY + 10, "LED Controller Status");

        // Body region (blue) — draw bars etc.
        for (int i = 0; i < 3; ++i)
        {
            const uint8_t y = bodyY + i * lineH;
            const char *label = (i == 0) ? "G" : (i == 1) ? "Y"
                                                          : "R";
            u8g2.drawStr(x0, y + (barH + 2), label);
            u8g2.drawFrame(x0 + 12, y, barW, barH);
            const uint8_t filled = (uint8_t)(barW * constrain(leds[i].activeRatio, 0.0f, 1.0f));
            if (filled > 0)
            {
                u8g2.drawBox(x0 + 12, y, filled, barH);
            }
            char buf[10];
            dtostrf(leds[i].activeRatio, 1, 2, buf);
            u8g2.drawStr(x0 + 12 + barW + 4, y + (barH + 2), buf);

            // Live state icon
            if (leds[i].state)
            {
                u8g2.drawDisc(120, y + barH / 2 + 1, 4);
            }
            else
            {
                u8g2.drawCircle(120, y + barH / 2 + 1, 4);
            }
        }
        // Period line (body region)
        char pbuf[20];
        sprintf(pbuf, "Period: %lums", leds[0].period);
        u8g2.drawStr(x0, bodyY + 3 * lineH + 4, pbuf);
    } while (u8g2.nextPage());
}
