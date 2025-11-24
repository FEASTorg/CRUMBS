#include <Arduino.h>
#include <U8g2lib.h>
#include "crumbs_arduino.h"
#include <stdio.h>

extern U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2;

extern const uint8_t LED_GREEN;
extern const uint8_t LED_YELLOW;
extern const uint8_t LED_RED;
extern const uint8_t kSliceI2cAddress;

extern volatile unsigned long yellowPulseUntil;
extern const unsigned long YELLOW_PULSE_MS;

extern crumbs_message_t lastRxMessage;
extern bool lastRxValid;

extern bool hasError;

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
    sprintf(head, "Peripheral 0x%02X", kSliceI2cAddress);
        u8g2.drawStr(0, 10, head);

        // Error/OK
        if (haveError)
            u8g2.drawStr(100, 10, "ERR");
        else
            u8g2.drawStr(100, 10, "OK ");

        int y = 24;
        if (!lastRxValid)
        {
            u8g2.drawStr(0, y, "No message yet");
            continue;
        }

        char line[26];
        sprintf(line, "type:%u cmd:%u crc:%02X", lastRxMessage.type_id, lastRxMessage.command_type, lastRxMessage.crc8);
        u8g2.drawStr(0, y, line);
        y += 12;

        char buf[32];
        snprintf(buf, sizeof(buf), "d0:%0.2f d1:%0.2f d2:%0.2f", lastRxMessage.data[0], lastRxMessage.data[1], lastRxMessage.data[2]);
        u8g2.drawStr(0, y, buf);
        y += 12;
        snprintf(buf, sizeof(buf), "d3:%0.2f d4:%0.2f", lastRxMessage.data[3], lastRxMessage.data[4]);
        u8g2.drawStr(0, y, buf);
        y += 12;
        snprintf(buf, sizeof(buf), "d5:%0.2f d6:%0.2f", lastRxMessage.data[5], lastRxMessage.data[6]);
        u8g2.drawStr(0, y, buf);
    } while (u8g2.nextPage());
}
