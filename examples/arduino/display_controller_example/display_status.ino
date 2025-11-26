#include <Arduino.h>
#include <U8g2lib.h>
#include "crumbs_arduino.h"
#include <stdio.h>

extern U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2;

extern const uint8_t LED_GREEN;
extern const uint8_t LED_YELLOW;
extern const uint8_t LED_RED;

extern volatile unsigned long yellowPulseUntil;
extern const unsigned long YELLOW_PULSE_MS;

extern struct LastExchange
{
    bool hasData;
    bool wasRx;
    uint8_t address;
    bool success;
    crumbs_message_t message;
} lastExchange;

extern bool hasError;

void setOk()
{
    haveError = false;
    digitalWrite(LED_RED, LOW);
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
    // Handle yellow pulse timing
    digitalWrite(LED_YELLOW, (millis() < yellowPulseUntil) ? HIGH : LOW);
}

void drawDisplay()
{
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_6x10_tf);

        u8g2.drawStr(0, 10, "CRUMBS Controller");
        u8g2.drawStr(0, 22, hasError ? "Status: ERROR" : "Status: OK");

        if (!lastExchange.hasData)
        {
            u8g2.drawStr(0, 34, "No message yet");
            continue;
        }

        // TX/RX line
        char line[20];
        int y = 34;
        sprintf(line, "%s 0x%02X", lastExchange.wasRx ? "RX" : "TX", lastExchange.address);
        u8g2.drawStr(0, y, line);
        y += 12;
        sprintf(line, "type:%u cmd:%u ok:%u", lastExchange.message.type_id, lastExchange.message.command_type, (uint8_t)lastExchange.success);
        u8g2.drawStr(0, y, line);
        y += 12;

        // data lines
        char buf[24];
        sprintf(buf, "d0:%0.2f d1:%0.2f", lastExchange.message.data[0], lastExchange.message.data[1]);
        u8g2.drawStr(0, y, buf);
        y += 12;
        sprintf(buf, "d2:%0.2f d3:%0.2f", lastExchange.message.data[2], lastExchange.message.data[3]);
        u8g2.drawStr(0, y, buf);
        y += 12;
        sprintf(buf, "d4:%0.2f d5:%0.2f", lastExchange.message.data[4], lastExchange.message.data[5]);
        u8g2.drawStr(0, y, buf);
    } while (u8g2.nextPage());
}
