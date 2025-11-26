#include <Arduino.h>
#include <crumbs_arduino.h>

// Minimal PlatformIO-compatible Arduino example showing CRUMBS controller init

crumbs_context_t controller_ctx;

void setup()
{
  Serial.begin(115200);
  while (!Serial) { delay(1); }
  Serial.println("CRUMBS PlatformIO example — initializing controller...");

  crumbs_arduino_init_controller(&controller_ctx);
  Serial.println("CRUMBS controller initialized");
}

void loop()
{
  // Demonstrate a no-op encoded message construction (doesn't require actual hardware)
  crumbs_message_t m = {};
  m.type_id = 0x01;
  m.command_type = 0x01;
  m.data[0] = 3.1415f;

  uint8_t buf[CRUMBS_MESSAGE_SIZE];
  size_t w = crumbs_encode_message(&m, buf, sizeof(buf));
  if (w == CRUMBS_MESSAGE_SIZE)
  {
    Serial.println("Encoded CRUMBS message successfully (demo)");
  }

  delay(2000);
}
