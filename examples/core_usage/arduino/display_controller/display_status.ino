/**
 * @file display_status.ino
 * @brief OLED display rendering for the controller example.
 *
 * This file provides drawDisplay() which is called from the main .ino.
 * All state variables are defined in display_controller.ino.
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include "crumbs_arduino.h"
#include <stdio.h>

// External references to state defined in display_controller.ino
extern U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2;
extern struct LastExchange lastExchange;
extern bool hasError;

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
        char line[24];
        int y = 34;
        sprintf(line, "%s 0x%02X", lastExchange.wasRx ? "RX" : "TX", lastExchange.address);
        u8g2.drawStr(0, y, line);
        y += 12;

        sprintf(line, "t:%u c:%u len:%u ok:%u",
                lastExchange.message.type_id,
                lastExchange.message.opcode,
                lastExchange.message.data_len,
                (uint8_t)lastExchange.success);
        u8g2.drawStr(0, y, line);
        y += 12;

        // Show data: first 3 bytes ... last 2 bytes
        char buf[24];
        uint8_t len = lastExchange.message.data_len;
        if (len == 0)
        {
            u8g2.drawStr(0, y, "d: (empty)");
        }
        else if (len <= 5)
        {
            // Show all bytes if 5 or fewer
            snprintf(buf, sizeof(buf), "d:%02X %02X %02X %02X %02X",
                     lastExchange.message.data[0],
                     len > 1 ? lastExchange.message.data[1] : 0,
                     len > 2 ? lastExchange.message.data[2] : 0,
                     len > 3 ? lastExchange.message.data[3] : 0,
                     len > 4 ? lastExchange.message.data[4] : 0);
            u8g2.drawStr(0, y, buf);
        }
        else
        {
            // Show first 3 ... last 2
            snprintf(buf, sizeof(buf), "d:%02X %02X %02X..%02X %02X",
                     lastExchange.message.data[0],
                     lastExchange.message.data[1],
                     lastExchange.message.data[2],
                     lastExchange.message.data[len - 2],
                     lastExchange.message.data[len - 1]);
            u8g2.drawStr(0, y, buf);
        }
    } while (u8g2.nextPage());
}
