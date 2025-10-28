// Arduino Nano + ELEGOO 0.96" SSD1306 OLED over software I2C on D7 (SDA), D8 (SCL)
// Libraries (Library Manager): "U8g2", "SPI", "Wire"
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

// Constructor: SW I2C, custom pins. Order: clock, data, reset.
// Use page buffer (_1_) to fit Nano RAM.
U8G2_SSD1306_128X64_NONAME_1_SW_I2C u8g2(
  U8G2_R0,           // rotation
  /* clock=*/8,      // SCL on D8
  /* data=*/7,       // SDA on D7
  /* reset=*/U8X8_PIN_NONE
);

// Non-blocking timer
unsigned long t0 = 0;
const unsigned long PERIOD_MS = 1000;
uint32_t count = 0;

void setup() {
  // Optional: set explicit I2C address; U8g2 expects 8-bit address value.
  // 0x3C << 1 == 0x78
  u8g2.setI2CAddress(0x3C << 1);

  u8g2.begin();  // initializes SW I2C on the given pins
}

void loop() {
  const unsigned long now = millis();
  if (now - t0 >= PERIOD_MS) {
    t0 = now;
    draw();
    count++;
  }
  // do other work here, no delays
}

void draw() {
  u8g2.firstPage();
  do {
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setCursor(0, 10);
    u8g2.print("Soft I2C D7/D8 OK");

    // Shapes
    u8g2.drawHLine(0, 14, 128);
    u8g2.drawFrame(0, 16, 30, 20);
    u8g2.drawBox(40, 16, 30, 20);
    u8g2.drawCircle(100, 26, 10);
    u8g2.drawDisc(100, 26, 5);

    // Counter
    u8g2.setFont(u8g2_font_logisoso16_tf);
    u8g2.setCursor(0, 60);
    u8g2.print("Count: ");
    u8g2.print(count);
  } while (u8g2.nextPage());
}
