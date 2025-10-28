// Arduino Nano + ELEGOO 0.96" SSD1306 OLED (yellow/blue) on software I2C (SCL=D8, SDA=D7)
// Non-blocking LED controller with serial commands and live OLED status
// Required libs: U8g2, SPI, Wire

#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

// ---------- OLED (software I2C) ----------
constexpr uint8_t OLED_ADDR = 0x3C;
constexpr uint8_t OLED_SCL = 8; // D8
constexpr uint8_t OLED_SDA = 7; // D7

U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(
    U8G2_R0, OLED_SCL, OLED_SDA, U8X8_PIN_NONE);

// ---------- LEDs ----------
struct Led
{
  uint8_t pin;
  float ratio;          // 0.0–1.0 active duty
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
constexpr unsigned long REPORT_MS = 1000;
constexpr unsigned long DISPLAY_MS = 100;

// ---------- Prototypes ----------
void handleSerial();
void updateLed(const String &, long, float);
void drawDisplay();

// =======================================================
void setup()
{
  Serial.begin(115200);
  for (auto &l : leds)
  {
    pinMode(l.pin, OUTPUT);
    digitalWrite(l.pin, LOW);
  }
  u8g2.setI2CAddress(OLED_ADDR << 1);
  u8g2.begin();

  Serial.println(F("Commands: SET <color> <period_ms> <ratio_0to1>"));
}

// =======================================================
void loop()
{
  const unsigned long now = millis();
  handleSerial();

  // LED timing logic
  for (auto &l : leds)
  {
    const unsigned long elapsed = now % l.period;
    const unsigned long active = (unsigned long)(l.period * l.ratio);
    const bool newState = (elapsed < active);
    if (newState != l.state)
    {
      l.state = newState;
      digitalWrite(l.pin, newState ? HIGH : LOW);
    }
  }

  if (now - lastReport >= REPORT_MS)
  {
    lastReport = now;
    Serial.print(F("G:"));
    Serial.print(leds[0].ratio, 2);
    Serial.print(F(" Y:"));
    Serial.print(leds[1].ratio, 2);
    Serial.print(F(" R:"));
    Serial.print(leds[2].ratio, 2);
    Serial.print(F("  P(ms):"));
    Serial.println(leds[0].period);
  }

  if (now - lastDisplay >= DISPLAY_MS)
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

  int a = cmd.indexOf(' '), b = cmd.indexOf(' ', a + 1), c = cmd.indexOf(' ', b + 1);
  if (a < 0 || b < 0 || c < 0)
  {
    Serial.println(F("Use: SET <color> <period_ms> <ratio>"));
    return;
  }

  String color = cmd.substring(a + 1, b);
  long period = cmd.substring(b + 1, c).toInt();
  float ratio = cmd.substring(c + 1).toFloat();
  updateLed(color, period, ratio);
}

// =======================================================
void updateLed(const String &color, long period, float ratio)
{
  int i = (color.equalsIgnoreCase("green")) ? 0 : (color.equalsIgnoreCase("yellow")) ? 1
                                              : (color.equalsIgnoreCase("red"))      ? 2
                                                                                     : -1;

  if (i < 0 || ratio < 0.0f || ratio > 1.0f || period <= 0)
  {
    Serial.println(F("Invalid parameters."));
    return;
  }
  leds[i].period = period;
  leds[i].ratio = ratio;
  Serial.print(color);
  Serial.print(F(" -> "));
  Serial.print(period);
  Serial.print(F(" ms, "));
  Serial.print(ratio, 2);
  Serial.println(F(" duty"));
}

// =======================================================
void drawDisplay()
{
  const uint8_t x0 = 0;
  const uint8_t yOffset = 4; // shifts below yellow/blue boundary
  const uint8_t headingY = yOffset;
  const uint8_t bodyY = headingY + 12;
  const uint8_t lineH = 14;
  const uint8_t barW = 76, barH = 8;

  u8g2.firstPage();
  do
  {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(x0, headingY + 10, "LED Status");

    for (int i = 0; i < 3; ++i)
    {
      const uint8_t y = bodyY + i * lineH;
      u8g2.drawStr(x0, y + barH + 2, (i == 0) ? "G" : (i == 1) ? "Y"
                                                               : "R");

      u8g2.drawFrame(x0 + 12, y, barW, barH);
      const uint8_t fill = (uint8_t)(barW * constrain(leds[i].ratio, 0.0f, 1.0f));
      if (fill)
        u8g2.drawBox(x0 + 12, y, fill, barH);

      char buf[6];
      dtostrf(leds[i].ratio, 1, 2, buf);
      u8g2.drawStr(x0 + 12 + barW + 4, y + barH + 2, buf);

      const uint8_t cy = y + barH / 2 + 1;
      if (leds[i].state)
        u8g2.drawDisc(120, cy, 4);
      else
        u8g2.drawCircle(120, cy, 4);
    }

    char pbuf[20];
    sprintf(pbuf, "P:%lums", leds[0].period);
    u8g2.drawStr(x0, bodyY + 3 * lineH + 4, pbuf);
  } while (u8g2.nextPage());
}
