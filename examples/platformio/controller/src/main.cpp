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

// Read a line from Serial and parse simple CSV commands in the form:
// addr,type_id,command_type,data0,data1,...
// Example: 0x08,1,1,3.14,1.0
void loop()
{
  if (!Serial.available())
  {
    delay(10);
    return;
  }

  char line[256] = {0};
  size_t len = Serial.readBytesUntil('\n', line, sizeof(line) - 1);
  line[len] = '\0';

  // Trim whitespace
  char *start = line;
  while (*start == ' ' || *start == '\r' || *start == '\t')
    ++start;
  if (*start == '\0')
    return;

  // Help
  if (strcmp(start, "help") == 0)
  {
    Serial.println(F("Usage: addr,type_id,command_type,data0,data1,.. (e.g. 0x08,1,1,3.14)"));
    return;
  }

  // Tokenize
  char *tok = strtok(start, ",");
  if (!tok)
  {
    Serial.println(F("Invalid input - expected CSV"));
    return;
  }

  long addr = strtol(tok, NULL, 0);
  tok = strtok(NULL, ",");
  if (!tok) { Serial.println(F("Missing type_id")); return; }
  int type_id = (int)strtol(tok, NULL, 0);
  tok = strtok(NULL, ",");
  if (!tok) { Serial.println(F("Missing command_type")); return; }
  int cmd = (int)strtol(tok, NULL, 0);

  crumbs_message_t m = {};
  m.type_id = (uint8_t)type_id;
  m.command_type = (uint8_t)cmd;

  // parse up to the payload length
  for (int i = 0; i < CRUMBS_DATA_LENGTH; ++i)
  {
    tok = strtok(NULL, ",");
    if (!tok)
      break;
    m.data[i] = (float)strtof(tok, NULL);
  }

  int rc = crumbs_controller_send(&controller_ctx, (uint8_t)addr, &m, crumbs_arduino_wire_write, NULL);
  if (rc == 0)
    Serial.println(F("Sent message successfully"));
  else
    Serial.print(F("Send failed rc=")), Serial.println(rc);
}
