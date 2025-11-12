#include <Arduino.h>
#include <U8g2lib.h>
#include <CRUMBS.h>
#include <stdio.h>

extern U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2;
extern bool hasError;
extern LastExchange lastExchange;

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
            u8g2.drawStr(0, 38, "No CRUMBS traffic yet");
        }
        else
        {
            char line[40];
            snprintf(line, sizeof(line), "%s 0x%02X %s",
                     lastExchange.wasRx ? "RX" : "TX",
                     lastExchange.address,
                     lastExchange.success ? "OK" : "FAIL");
            u8g2.drawStr(0, 38, line);

            snprintf(line, sizeof(line), "crc:%02X period:%lu",
                     lastExchange.message.crc8,
                     static_cast<unsigned long>(lastExchange.message.data[3]));
            u8g2.drawStr(0, 50, line);

            char dataLine[32];
            char g[6], y[6], r[6];
            dtostrf(lastExchange.message.data[0], 4, 2, g);
            dtostrf(lastExchange.message.data[1], 4, 2, y);
            dtostrf(lastExchange.message.data[2], 4, 2, r);
            snprintf(dataLine, sizeof(dataLine), "GYR %s %s %s", g, y, r);
            u8g2.drawStr(0, 62, dataLine);
        }
    } while (u8g2.nextPage());
}
