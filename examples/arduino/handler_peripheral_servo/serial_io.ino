/**
 * @file serial_io.ino
 * @brief Serial command interface for testing servo hardware.
 *
 * This file is conditionally compiled when ENABLE_SERIAL_TEST=1
 * in the main sketch. It provides direct serial control of servos
 * for hardware testing before integrating with I2C.
 *
 * Commands (case-insensitive):
 *   help           - Show this help
 *   status         - Show current servo angles
 *   set <ch> <deg> - Set servo ch (0-1) to angle (0-180)
 *   both <a0> <a1> - Set both servos
 *   sweep <ch> <start> <end> <ms> - Sweep servo
 *   center         - Center all servos (90°)
 *   test           - Run servo test sequence
 */

#if ENABLE_SERIAL_TEST

static char serial_buf[32];
static uint8_t serial_idx = 0;

/**
 * @brief Print help message.
 */
static void print_help()
{
    Serial.println(F("\n=== Servo Serial Test Mode ==="));
    Serial.println(F("Commands:"));
    Serial.println(F("  help                     - Show this help"));
    Serial.println(F("  status                   - Show servo angles"));
    Serial.println(F("  set <ch> <deg>           - Set servo (0-1) to angle"));
    Serial.println(F("  both <a0> <a1>           - Set both servos"));
    Serial.println(F("  sweep <ch> <s> <e> <ms>  - Sweep start->end"));
    Serial.println(F("  center                   - All servos to 90°"));
    Serial.println(F("  test                     - Run test sequence"));
    Serial.println();
}

/**
 * @brief Print current servo state.
 */
static void print_status()
{
    Serial.print(F("Servo angles: "));
    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        Serial.print(F("ch"));
        Serial.print(i);
        Serial.print(F("="));
        Serial.print(servo_angles[i]);
        Serial.print(F("°"));
        if (i < NUM_SERVOS - 1)
            Serial.print(F(", "));
    }
    Serial.println();
}

/**
 * @brief Run servo test sequence.
 */
static void run_test()
{
    Serial.println(F("Running servo test sequence..."));

    /* Center all */
    Serial.println(F("  Center all"));
    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        set_servo_angle(i, 90);
    }
    delay(500);

    /* Min position */
    Serial.println(F("  Min (0°)"));
    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        set_servo_angle(i, 0);
    }
    delay(500);

    /* Max position */
    Serial.println(F("  Max (180°)"));
    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        set_servo_angle(i, 180);
    }
    delay(500);

    /* Sweep each servo */
    for (uint8_t ch = 0; ch < NUM_SERVOS; ch++)
    {
        Serial.print(F("  Sweep ch"));
        Serial.println(ch);
        sweep_servo(ch, 180, 0, 10);
        delay(200);
        sweep_servo(ch, 0, 90, 10);
    }

    /* Alternating */
    Serial.println(F("  Alternating"));
    for (uint8_t i = 0; i < 4; i++)
    {
        set_servo_angle(0, 45);
        set_servo_angle(1, 135);
        delay(400);
        set_servo_angle(0, 135);
        set_servo_angle(1, 45);
        delay(400);
    }

    /* Return to center */
    Serial.println(F("  Center all"));
    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        set_servo_angle(i, 90);
    }
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
    else if (strncmp(cmd, "center", 6) == 0)
    {
        for (uint8_t i = 0; i < NUM_SERVOS; i++)
        {
            set_servo_angle(i, 90);
        }
        Serial.println(F("All servos centered (90°)"));
        print_status();
    }
    else if (strncmp(cmd, "set ", 4) == 0)
    {
        uint8_t ch = (uint8_t)atoi(cmd + 4);
        char *p = strchr(cmd + 4, ' ');
        if (p && ch < NUM_SERVOS)
        {
            uint8_t angle = (uint8_t)atoi(p + 1);
            if (angle > 180)
                angle = 180;
            set_servo_angle(ch, angle);
            print_status();
        }
        else
        {
            Serial.println(F("Usage: set <0-1> <0-180>"));
        }
    }
    else if (strncmp(cmd, "both ", 5) == 0)
    {
        uint8_t a0 = (uint8_t)atoi(cmd + 5);
        char *p = strchr(cmd + 5, ' ');
        if (p)
        {
            uint8_t a1 = (uint8_t)atoi(p + 1);
            if (a0 > 180)
                a0 = 180;
            if (a1 > 180)
                a1 = 180;
            set_servo_angle(0, a0);
            set_servo_angle(1, a1);
            print_status();
        }
        else
        {
            Serial.println(F("Usage: both <a0> <a1>"));
        }
    }
    else if (strncmp(cmd, "sweep ", 6) == 0)
    {
        /* Parse: sweep <ch> <start> <end> <ms> */
        char *args = cmd + 6;
        uint8_t ch = (uint8_t)atoi(args);

        char *p1 = strchr(args, ' ');
        char *p2 = p1 ? strchr(p1 + 1, ' ') : nullptr;
        char *p3 = p2 ? strchr(p2 + 1, ' ') : nullptr;

        if (p3 && ch < NUM_SERVOS)
        {
            uint8_t start = (uint8_t)atoi(p1 + 1);
            uint8_t end = (uint8_t)atoi(p2 + 1);
            uint8_t step_ms = (uint8_t)atoi(p3 + 1);

            if (start > 180)
                start = 180;
            if (end > 180)
                end = 180;
            if (step_ms < 1)
                step_ms = 1;

            Serial.print(F("Sweeping ch"));
            Serial.print(ch);
            Serial.print(F(": "));
            Serial.print(start);
            Serial.print(F("° -> "));
            Serial.print(end);
            Serial.print(F("° @ "));
            Serial.print(step_ms);
            Serial.println(F("ms/step"));

            sweep_servo(ch, start, end, step_ms);
            print_status();
        }
        else
        {
            Serial.println(F("Usage: sweep <ch> <start> <end> <ms>"));
        }
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
