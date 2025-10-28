// Simple non-blocking LED controller with serial commands
// Green LED: D4, Yellow LED: D6, Red LED: D5

struct Led
{
    uint8_t pin;
    float activeRatio;    // 0.0–1.0
    unsigned long period; // ms
    unsigned long lastToggle;
    bool state;
};

Led leds[3] = {
    {4, 0.5, 2000, 0, false}, // Green
    {6, 0.5, 2000, 0, false}, // Yellow
    {5, 0.5, 2000, 0, false}  // Red
};

unsigned long lastReport = 0;

void setup()
{
    Serial.begin(115200);
    for (auto &led : leds)
    {
        pinMode(led.pin, OUTPUT);
        digitalWrite(led.pin, LOW);
    }
    Serial.println("Ready. Commands:");
    Serial.println("SET <color> <period_ms> <ratio_0to1>");
    Serial.println("Example: SET green 1000 0.25");
}

void loop()
{
    unsigned long now = millis();

    // Handle serial input
    handleSerial();

    // Update LEDs non-blocking
    for (auto &led : leds)
    {
        unsigned long activeTime = led.period * led.activeRatio;
        unsigned long elapsed = now % led.period;
        bool newState = (elapsed < activeTime);
        if (newState != led.state)
        {
            led.state = newState;
            digitalWrite(led.pin, newState ? HIGH : LOW);
        }
    }

    // Status report every second
    if (now - lastReport >= 1000)
    {
        lastReport = now;
        Serial.print("Status | ");
        Serial.print("G:");
        Serial.print(leds[0].activeRatio);
        Serial.print(" Y:");
        Serial.print(leds[1].activeRatio);
        Serial.print(" R:");
        Serial.print(leds[2].activeRatio);
        Serial.print(" Period(ms):");
        Serial.print(leds[0].period);
        Serial.println();
    }
}

void handleSerial()
{
    if (Serial.available())
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd.startsWith("SET"))
        {
            String color, periodStr, ratioStr;
            int firstSpace = cmd.indexOf(' ');
            int secondSpace = cmd.indexOf(' ', firstSpace + 1);
            int thirdSpace = cmd.indexOf(' ', secondSpace + 1);
            if (secondSpace > 0 && thirdSpace > 0)
            {
                color = cmd.substring(firstSpace + 1, secondSpace);
                periodStr = cmd.substring(secondSpace + 1, thirdSpace);
                ratioStr = cmd.substring(thirdSpace + 1);
                long period = periodStr.toInt();
                float ratio = ratioStr.toFloat();
                updateLed(color, period, ratio);
            }
        }
    }
}

void updateLed(String color, long period, float ratio)
{
    int idx = -1;
    if (color.equalsIgnoreCase("green"))
        idx = 0;
    else if (color.equalsIgnoreCase("yellow"))
        idx = 1;
    else if (color.equalsIgnoreCase("red"))
        idx = 2;

    if (idx >= 0 && ratio >= 0.0 && ratio <= 1.0 && period > 0)
    {
        leds[idx].period = period;
        leds[idx].activeRatio = ratio;
        Serial.print("Updated ");
        Serial.print(color);
        Serial.print(": period=");
        Serial.print(period);
        Serial.print("ms ratio=");
        Serial.println(ratio, 2);
    }
    else
    {
        Serial.println("Invalid parameters.");
    }
}
