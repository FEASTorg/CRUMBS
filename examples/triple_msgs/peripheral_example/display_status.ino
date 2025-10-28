#include <Arduino.h>
#include <U8g2lib.h>

extern U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2;

extern const uint8_t LED_GREEN;
extern const uint8_t LED_YELLOW;
extern const uint8_t LED_RED;

extern volatile unsigned long yellowPulseUntil;
extern const unsigned long YELLOW_PULSE_MS;

extern struct LastMsg
{
    uint8_t typeID;
    uint8_t commandType;
    float data[6];
    uint8_t errorFlags;
    bool valid;
} lastRx;

static bool haveError = false;

void setOk()
{
    haveError = false;
    digitalWrite(LED_RED, LOW);
    // Green LED is also used by CRUMBS-controlled channel; keep on when OK:
    digitalWrite(LED_GREEN, HIGH);
}

void setError()
{
    haveError = true;
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
}

void pulseActivity()
{
    yellowPulseUntil = millis() + YELLOW_PULSE_MS;
}

void refreshLeds()
{
    digitalWrite(LED_YELLOW, (millis() < yellowPulseUntil) ? HIGH : LOW);
}

void drawDisplay()
{
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_6x10_tf);

        // Header "Peripheral 0x.."
        char head[20];
        extern uint8_t SLICE_I2C_ADDRESS;
        sprintf(head, "Peripheral 0x%02X", SLICE_I2C_ADDRESS);
        u8g2.drawStr(0, 10, head);

        // Error/OK
        if (haveError)
            u8g2.drawStr(100, 10, "ERR");
        else
            u8g2.drawStr(100, 10, "OK ");

        int y = 24;
        if (!lastRx.valid)
        {
            u8g2.drawStr(0, y, "No message yet");
            continue;
        }

        char line[26];
        sprintf(line, "type:%u cmd:%u err:%u", lastRx.typeID, lastRx.commandType, lastRx.errorFlags);
        u8g2.drawStr(0, y, line);
        y += 12;

        char buf[26];
        sprintf(buf, "d0:%0.2f d1:%0.2f", lastRx.data[0], lastRx.data[1]);
        u8g2.drawStr(0, y, buf);
        y += 12;
        sprintf(buf, "d2:%0.2f d3:%0.2f", lastRx.data[2], lastRx.data[3]);
        u8g2.drawStr(0, y, buf);
        y += 12;
        sprintf(buf, "d4:%0.2f d5:%0.2f", lastRx.data[4], lastRx.data[5]);
        u8g2.drawStr(0, y, buf);
    } while (u8g2.nextPage());
}
