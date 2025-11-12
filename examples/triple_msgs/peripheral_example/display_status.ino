#include <Arduino.h>
#include <U8g2lib.h>
#include <CRUMBS.h>
#include <stdio.h>

extern U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2;
extern const uint8_t kSliceI2cAddress;
extern bool hasError;
extern LedChannel ledChannels[3];
extern MessageSummary lastCommand;
extern MessageSummary lastResponse;

void drawDisplay()
{
    u8g2.firstPage();
    do
    {
        u8g2.setFont(u8g2_font_6x10_tf);

        char header[24];
        snprintf(header, sizeof(header), "Slice 0x%02X", kSliceI2cAddress);
        u8g2.drawStr(0, 10, header);
        u8g2.drawStr(0, 23, hasError ? "Status: ERROR" : "Status: OK");

        if (lastCommand.valid)
        {
            char line[40];
            char g[6], y[6], r[6];
            dtostrf(lastCommand.message.data[0], 4, 2, g);
            dtostrf(lastCommand.message.data[1], 4, 2, y);
            dtostrf(lastCommand.message.data[2], 4, 2, r);
            snprintf(line, sizeof(line), "Cmd C %s %s %s", g, y, r);
            u8g2.drawStr(0, 38, line);

            snprintf(line, sizeof(line), "Cmd period %lu ms",
                     static_cast<unsigned long>(lastCommand.message.data[3]));
            u8g2.drawStr(0, 50, line);
        }
        else
        {
            u8g2.drawStr(0, 38, "Awaiting controller...");
        }

        char stateLine[40];
        char g[6], y[6], r[6];
        dtostrf(ledChannels[0].ratio, 4, 2, g);
        dtostrf(ledChannels[1].ratio, 4, 2, y);
        dtostrf(ledChannels[2].ratio, 4, 2, r);
        if (lastResponse.valid)
        {
            snprintf(stateLine, sizeof(stateLine), "LED %s %s %s rsp:%02X",
                     g, y, r, lastResponse.message.crc8);
        }
        else
        {
            snprintf(stateLine, sizeof(stateLine), "LED %s %s %s", g, y, r);
        }
        u8g2.drawStr(0, 62, stateLine);
    } while (u8g2.nextPage());
}
