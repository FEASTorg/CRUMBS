/**
 * @file display_status.ino
 * @brief OLED display and LED helpers for the peripheral example.
 *
 * Provides drawDisplay(), refreshLeds(), pulseActivity(), setOk(), setError().
 * All state variables are defined in display_peripheral.ino.
 */

#include <Arduino.h>
#include <U8g2lib.h>
#include "crumbs_arduino.h"
#include <stdio.h>

// External references to state defined in display_peripheral.ino
extern U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2;
extern const uint8_t LED_GREEN;
extern const uint8_t LED_YELLOW;
extern const uint8_t LED_RED;
extern const uint8_t kSliceI2cAddress;
extern volatile unsigned long yellowPulseUntil;
extern const unsigned long YELLOW_PULSE_MS;
extern unsigned long errorBlinkLast;
extern const unsigned long ERROR_BLINK_MS;
extern bool errorBlinkState;
extern crumbs_message_t lastRxMessage;
extern bool lastRxValid;
extern bool hasError;

void setOk()
{
    hasError = false;
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
}

void setError()
{
    hasError = true;
    digitalWrite(LED_GREEN, LOW);
    // Red will blink in refreshLeds()
}

void pulseActivity()
{
    yellowPulseUntil = millis() + YELLOW_PULSE_MS;
}

void refreshLeds()
{
    // Yellow: activity pulse
    digitalWrite(LED_YELLOW, (millis() < yellowPulseUntil) ? HIGH : LOW);

    // Red: blink when error
    if (hasError)
    {
        unsigned long now = millis();
        if (now - errorBlinkLast >= ERROR_BLINK_MS)
        {
            errorBlinkLast = now;
            errorBlinkState = !errorBlinkState;
            digitalWrite(LED_RED, errorBlinkState ? HIGH : LOW);
        }
    }
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
        if (hasError)
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
        sprintf(line, "t:%u c:%u len:%u crc:%02X",
                lastRxMessage.type_id,
                lastRxMessage.command_type,
                lastRxMessage.data_len,
                lastRxMessage.crc8);
        u8g2.drawStr(0, y, line);
        y += 12;

        // Show data across 3 lines, 7 bytes each (max 21 shown)
        // If len > 21: show first 20, then .., then last byte
        char buf[24];
        uint8_t len = lastRxMessage.data_len;

        if (len == 0)
        {
            u8g2.drawStr(0, y, "(no data)");
        }
        else
        {
            // Line 1: bytes 0-6
            uint8_t show = (len > 7) ? 7 : len;
            snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X %02X %02X",
                     lastRxMessage.data[0],
                     show > 1 ? lastRxMessage.data[1] : 0,
                     show > 2 ? lastRxMessage.data[2] : 0,
                     show > 3 ? lastRxMessage.data[3] : 0,
                     show > 4 ? lastRxMessage.data[4] : 0,
                     show > 5 ? lastRxMessage.data[5] : 0,
                     show > 6 ? lastRxMessage.data[6] : 0);
            u8g2.drawStr(0, y, buf);
            y += 12;

            if (len > 7)
            {
                // Line 2: bytes 7-13
                show = (len > 14) ? 7 : (len - 7);
                snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X %02X %02X",
                         lastRxMessage.data[7],
                         show > 1 ? lastRxMessage.data[8] : 0,
                         show > 2 ? lastRxMessage.data[9] : 0,
                         show > 3 ? lastRxMessage.data[10] : 0,
                         show > 4 ? lastRxMessage.data[11] : 0,
                         show > 5 ? lastRxMessage.data[12] : 0,
                         show > 6 ? lastRxMessage.data[13] : 0);
                u8g2.drawStr(0, y, buf);
                y += 12;
            }

            if (len > 14)
            {
                // Line 3: bytes 14-19, then .. and last byte if len > 20
                if (len <= 21)
                {
                    // Show remaining bytes (up to 7)
                    show = len - 14;
                    snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X %02X %02X",
                             lastRxMessage.data[14],
                             show > 1 ? lastRxMessage.data[15] : 0,
                             show > 2 ? lastRxMessage.data[16] : 0,
                             show > 3 ? lastRxMessage.data[17] : 0,
                             show > 4 ? lastRxMessage.data[18] : 0,
                             show > 5 ? lastRxMessage.data[19] : 0,
                             show > 6 ? lastRxMessage.data[20] : 0);
                    u8g2.drawStr(0, y, buf);
                }
                else
                {
                    // Show bytes 14-19, then .., then last byte
                    snprintf(buf, sizeof(buf), "%02X %02X %02X %02X %02X %02X..%02X",
                             lastRxMessage.data[14],
                             lastRxMessage.data[15],
                             lastRxMessage.data[16],
                             lastRxMessage.data[17],
                             lastRxMessage.data[18],
                             lastRxMessage.data[19],
                             lastRxMessage.data[len - 1]);
                    u8g2.drawStr(0, y, buf);
                }
            }
        }
    } while (u8g2.nextPage());
}
