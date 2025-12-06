/**
 * @file serial_io.ino
 * @brief Serial command interface for testing LED hardware.
 *
 * This file is conditionally compiled when ENABLE_SERIAL_TEST=1
 * in the main sketch. It provides direct serial control of LEDs
 * for hardware testing before integrating with I2C.
 *
 * Commands (case-insensitive):
 *   help           - Show this help
 *   status         - Show current LED state
 *   all <bitmask>  - Set all LEDs (0-15 or 0b1111)
 *   set <n> <0|1>  - Set LED n (0-3) on/off
 *   on <n>         - Turn LED n on
 *   off <n>        - Turn LED n off
 *   blink <count> <delay_ms> - Blink all LEDs
 *   test           - Run LED test sequence
 */

#if ENABLE_SERIAL_TEST

static char serial_buf[32];
static uint8_t serial_idx = 0;

/**
 * @brief Print help message.
 */
static void print_help()
{
    Serial.println(F("\n=== LED Serial Test Mode ==="));
    Serial.println(F("Commands:"));
    Serial.println(F("  help           - Show this help"));
    Serial.println(F("  status         - Show current LED state"));
    Serial.println(F("  all <0-15>     - Set all LEDs (bitmask)"));
    Serial.println(F("  set <n> <0|1>  - Set LED n (0-3) on/off"));
    Serial.println(F("  on <n>         - Turn LED n on"));
    Serial.println(F("  off <n>        - Turn LED n off"));
    Serial.println(F("  blink <count> <ms> - Blink all LEDs"));
    Serial.println(F("  test           - Run test sequence"));
    Serial.println();
}

/**
 * @brief Print current LED state.
 */
static void print_status()
{
    Serial.print(F("LED state: 0b"));
    for (int8_t i = NUM_LEDS - 1; i >= 0; i--)
    {
        Serial.print((led_state >> i) & 1);
    }
    Serial.print(F(" ("));
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        Serial.print((led_state >> i) & 1 ? F("ON") : F("--"));
        if (i < NUM_LEDS - 1)
            Serial.print(F(" "));
    }
    Serial.println(F(")"));
}

/**
 * @brief Run LED test sequence.
 */
static void run_test()
{
    Serial.println(F("Running LED test sequence..."));

    /* All off */
    apply_state(0);
    delay(300);

    /* Walk through each LED */
    for (uint8_t i = 0; i < NUM_LEDS; i++)
    {
        Serial.print(F("  LED "));
        Serial.println(i);
        apply_state(1 << i);
        delay(300);
    }

    /* All on */
    Serial.println(F("  All ON"));
    apply_state(0x0F);
    delay(500);

    /* Blink */
    Serial.println(F("  Blink"));
    for (uint8_t i = 0; i < 3; i++)
    {
        apply_state(0);
        delay(150);
        apply_state(0x0F);
        delay(150);
    }

    /* Binary count */
    Serial.println(F("  Binary count"));
    for (uint8_t i = 0; i <= 15; i++)
    {
        apply_state(i);
        delay(150);
    }

    /* Return to off */
    apply_state(0);
    Serial.println(F("Test complete."));
}

/**
 * @brief Parse and execute a command.
 */
static void process_command(char *cmd)
{
    /* Trim leading spaces */
    while (*cmd == ' ')
        cmd++;

    /* Empty command */
    if (*cmd == '\0')
        return;

    /* Convert to lowercase for comparison */
    for (char *p = cmd; *p && *p != ' '; p++)
    {
        if (*p >= 'A' && *p <= 'Z')
            *p += 32;
    }

    if (strncmp(cmd, "help", 4) == 0)
    {
        print_help();
    }
    else if (strncmp(cmd, "status", 6) == 0)
    {
        print_status();
    }
    else if (strncmp(cmd, "test", 4) == 0)
    {
        run_test();
    }
    else if (strncmp(cmd, "all ", 4) == 0)
    {
        uint8_t val = (uint8_t)atoi(cmd + 4);
        apply_state(val & 0x0F);
        print_status();
    }
    else if (strncmp(cmd, "set ", 4) == 0)
    {
        uint8_t n = (uint8_t)atoi(cmd + 4);
        char *p = strchr(cmd + 4, ' ');
        if (p && n < NUM_LEDS)
        {
            uint8_t v = (uint8_t)atoi(p + 1);
            if (v)
            {
                led_state |= (1 << n);
            }
            else
            {
                led_state &= ~(1 << n);
            }
            apply_state(led_state);
            print_status();
        }
        else
        {
            Serial.println(F("Usage: set <0-3> <0|1>"));
        }
    }
    else if (strncmp(cmd, "on ", 3) == 0)
    {
        uint8_t n = (uint8_t)atoi(cmd + 3);
        if (n < NUM_LEDS)
        {
            led_state |= (1 << n);
            apply_state(led_state);
            print_status();
        }
        else
        {
            Serial.println(F("Usage: on <0-3>"));
        }
    }
    else if (strncmp(cmd, "off ", 4) == 0)
    {
        uint8_t n = (uint8_t)atoi(cmd + 4);
        if (n < NUM_LEDS)
        {
            led_state &= ~(1 << n);
            apply_state(led_state);
            print_status();
        }
        else
        {
            Serial.println(F("Usage: off <0-3>"));
        }
    }
    else if (strncmp(cmd, "blink ", 6) == 0)
    {
        uint8_t count = (uint8_t)atoi(cmd + 6);
        char *p = strchr(cmd + 6, ' ');
        uint16_t delay_ms = p ? (uint16_t)atoi(p + 1) : 200;

        Serial.print(F("Blinking "));
        Serial.print(count);
        Serial.print(F(" times, "));
        Serial.print(delay_ms);
        Serial.println(F("ms delay"));

        uint8_t saved = led_state;
        for (uint8_t i = 0; i < count; i++)
        {
            apply_state(0x0F);
            delay(delay_ms);
            apply_state(0);
            delay(delay_ms);
        }
        apply_state(saved);
    }
    else
    {
        Serial.print(F("Unknown command: "));
        Serial.println(cmd);
        Serial.println(F("Type 'help' for commands."));
    }
}

/**
 * @brief Setup for serial test mode.
 */
void serial_io_setup()
{
    print_help();
    Serial.print(F("> "));
}

/**
 * @brief Loop for serial test mode.
 */
void serial_io_loop()
{
    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\n' || c == '\r')
        {
            if (serial_idx > 0)
            {
                serial_buf[serial_idx] = '\0';
                Serial.println();
                process_command(serial_buf);
                serial_idx = 0;
                Serial.print(F("> "));
            }
        }
        else if (c == '\b' || c == 127)
        {
            /* Backspace */
            if (serial_idx > 0)
            {
                serial_idx--;
                Serial.print(F("\b \b"));
            }
        }
        else if (serial_idx < sizeof(serial_buf) - 1)
        {
            serial_buf[serial_idx++] = c;
            Serial.print(c);
        }
    }
}

#endif /* ENABLE_SERIAL_TEST */
