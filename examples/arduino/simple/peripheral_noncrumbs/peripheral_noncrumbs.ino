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
 * `scan` or Linux `scan`) — this peripheral intentionally does NOT implement
 * the CRUMBS protocol.
 */

#include <Arduino.h>
#include <Wire.h>

// Choose address 69 decimal (0x45)
#define NONCRUMBS_ADDR 69

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
    const char *payload = "NOCRUMBS"; // 8 bytes
    Wire.write((const uint8_t *)payload, strlen(payload));
}

void setup()
{
    Serial.begin(115200);
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
    // Nothing to do here — interrupts drive I2C callbacks.
    delay(1000);
}
