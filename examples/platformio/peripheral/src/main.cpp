/**
 * @file
 * @brief Small PlatformIO Arduino peripheral demo for CRUMBS.
 */

#include <Arduino.h>
#include <crumbs_arduino.h>

crumbs_context_t per_ctx;

static void on_message(crumbs_context_t *ctx, const crumbs_message_t *m)
{
  Serial.print(F("Received message: type_id="));
  Serial.print(m->type_id);
  Serial.print(F(" cmd="));
  Serial.println(m->command_type);
  for (size_t i = 0; i < CRUMBS_DATA_LENGTH; ++i)
  {
    // Print payload fields; demo stops at the first zero float
    if (m->data[i] == 0.0f) break;
    Serial.print(F(" data[")); Serial.print(i); Serial.print(F("]="));
    Serial.print(m->data[i], 4);
  }
  Serial.println();
}

static void on_request(crumbs_context_t *ctx, crumbs_message_t *reply)
{
  // Provide a simple reply payload for demo purposes
  reply->type_id = 0x10;
  reply->command_type = 0x42; // arbitrary
  for (size_t i = 0; i < CRUMBS_DATA_LENGTH; ++i)
    reply->data[i] = (float)(i + 1) * 1.234f;
}

void setup()
{
  Serial.begin(115200);
  while (!Serial) { delay(1); }
  Serial.println("CRUMBS PlatformIO peripheral (Nano) — init at address 0x08");

  crumbs_arduino_init_peripheral(&per_ctx, 0x08);
  crumbs_set_callbacks(&per_ctx, on_message, on_request, NULL);
}

void loop()
{
  // Peripheral work is interrupt-driven by Arduino Wire callbacks; use loop to blink
  digitalWrite(LED_BUILTIN, millis() % 1000 < 50);
  delay(50);
}
