/*
 * peripheral_noncrumbs.ino
 *
 * Minimal Arduino 'non-CRUMBS' peripheral for testing scanner false-positives.
 *
 * Behavior:
 *  - Joins I2C as a slave at address 69 (0x45)
 *  - On receive: drains the bytes and logs count to Serial
 *  - On request: responds with a short, non-CRUMBS payload (will NOT be a
 *    valid 31-byte CRUMBS frame) so the CRUMBS-aware scanner should *not*
 *    identify this device as CRUMBS.
 *
 * Usage: Upload to your microcontroller and run the scanner (Arduino serial
 * `scan` or Linux `scan`) - this peripheral intentionally does NOT implement
 * the CRUMBS protocol. Used to test the crumbs device auto-scanner.
 */

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

void onReceiveHandler(int numBytes)
{
    // Drain and count bytes (we intentionally do not interpret them as CRUMBS)
    int count = 0;
    while (Wire.available())
    {
        (void)Wire.read();
        ++count;
    }
    Serial.print(F("non-CRUMBS peripheral: received bytes="));
    Serial.println(count);
}

void onRequestHandler()
{
    // Respond with a short, non-CRUMBS payload (e.g., a simple text token)
    // This is intentionally not 31 bytes and does not contain a valid CRC.
    Wire.write((const uint8_t *)NONCRUMBS_PAYLOAD, strlen(NONCRUMBS_PAYLOAD));
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    while (!Serial)
        ; // wait for serial monitor on some boards

    Serial.print(F("Starting non-CRUMBS peripheral at address 0x"));
    Serial.println(NONCRUMBS_ADDR, HEX);

    Wire.begin(NONCRUMBS_ADDR);
    Wire.onReceive(onReceiveHandler);
    Wire.onRequest(onRequestHandler);
}

void loop()
{
    // Nothing to do here - interrupts drive I2C callbacks.
    delay(LOOP_DELAY_MS);
}
