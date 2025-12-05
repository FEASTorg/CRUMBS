/**
 * @file led_peripheral_minimal.ino
 * @brief Minimal test to debug startup crash.
 */

/*
 * NOTE: CRUMBS_MAX_HANDLERS must be set in platformio.ini build_flags,
 * not here, because Arduino precompiles the library separately.
 * Default is 16 handlers (~70 bytes on AVR).
 */

#include <Wire.h>
#include <crumbs.h>

#define I2C_ADDRESS 0x08

static const uint8_t LED_PINS[] = {4, 5, 6, 7};
static const uint8_t NUM_LEDS = 4;

/* Static initialization ensures zero-init (critical for handler arrays) */
static crumbs_context_t ctx;

void setup()
{
    Serial.begin(115200);
    delay(100);  /* Give serial time to initialize */
    
    Serial.println(F("1. Serial OK"));
    Serial.flush();
    
    /* Initialize LED pins */
    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        pinMode(LED_PINS[i], OUTPUT);
        digitalWrite(LED_PINS[i], LOW);
    }
    Serial.println(F("2. Pins OK"));
    Serial.flush();
    
    /* Print context size for debugging */
    Serial.print(F("3. ctx size (sketch): "));
    Serial.println(sizeof(ctx));
    Serial.print(F("   ctx size (library): "));
    Serial.println(crumbs_context_size());
    Serial.flush();
    
    /* Verify sizes match - critical for Arduino! */
    if (sizeof(ctx) != crumbs_context_size()) {
        Serial.println(F("ERROR: Size mismatch! Use build_flags."));
        Serial.println(F("Add to platformio.ini:"));
        Serial.println(F("  build_flags = -DCRUMBS_MAX_HANDLERS=8"));
        while (1) { delay(1000); }  /* Halt */
    }
    Serial.println(F("4. Size check OK"));
    Serial.flush();
    
    /* Initialize CRUMBS context manually (no HAL) */
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, I2C_ADDRESS);
    Serial.println(F("5. crumbs_init OK"));
    Serial.flush();
    
    /* Initialize Wire as peripheral */
    Wire.begin(I2C_ADDRESS);
    Serial.println(F("6. Wire.begin OK"));
    Serial.flush();
    
    Serial.println(F("=== Minimal Test Complete ==="));
}

void loop()
{
    /* Blink LED 0 to show we're alive */
    static unsigned long lastBlink = 0;
    static bool ledState = false;
    
    if (millis() - lastBlink > 500) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(LED_PINS[0], ledState ? HIGH : LOW);
    }
}
