/**
 * @file main.cpp
 * @brief Interactive mock device controller.
 *
 * This example demonstrates:
 * - Using helper functions from mock_ops.h
 * - Interactive serial command interface
 * - Controller-side command sending
 * - SET_REPLY pattern for queries
 *
 * Hardware:
 * - Arduino Nano (or compatible) as I2C controller
 * - Mock peripheral at address 0x08
 *
 * Serial Commands:
 * - help                    - Show available commands
 * - echo <hex bytes>        - Send echo data (e.g., "echo DE AD BE EF")
 * - heartbeat <ms>          - Set heartbeat period in milliseconds
 * - toggle                  - Toggle heartbeat enable/disable
 * - status                  - Query heartbeat status and period
 * - getecho                 - Query stored echo data
 * - info                    - Query device info
 * - scan                    - Scan I2C bus
 */

#include <Arduino.h>
#include <crumbs.h>
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

/* Include contract header (from parent directory) */
#include "mock_ops.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define PERIPHERAL_ADDR 0x08
#define MAX_CMD_LEN 128

/* ============================================================================
 * State
 * ============================================================================ */

static crumbs_context_t ctx;
static char cmd_buffer[MAX_CMD_LEN];
static uint8_t cmd_len = 0;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Print help text.
 */
static void print_help()
{
    Serial.println(F("\n=== Mock Controller Commands ==="));
    Serial.println(F("  help                   - Show this help"));
    Serial.println(F("  echo <hex bytes>       - Send echo data (e.g., 'echo DE AD BE EF')"));
    Serial.println(F("  heartbeat <ms>         - Set heartbeat period (e.g., 'heartbeat 1000')"));
    Serial.println(F("  toggle                 - Toggle heartbeat enable/disable"));
    Serial.println(F("  status                 - Query heartbeat status and period"));
    Serial.println(F("  getecho                - Query stored echo data"));
    Serial.println(F("  info                   - Query device info"));
    Serial.println(F("  scan                   - Scan I2C bus"));
    Serial.println();
}

/**
 * @brief Query peripheral and print reply.
 */
static int query_and_print(uint8_t query_op, const char *label)
{
    int rc;

    /* Send query using helper function */
    if (query_op == MOCK_OP_GET_ECHO)
    {
        rc = mock_query_echo(&ctx, PERIPHERAL_ADDR, crumbs_arduino_write, &Wire);
    }
    else if (query_op == MOCK_OP_GET_STATUS)
    {
        rc = mock_query_status(&ctx, PERIPHERAL_ADDR, crumbs_arduino_write, &Wire);
    }
    else if (query_op == MOCK_OP_GET_INFO)
    {
        rc = mock_query_info(&ctx, PERIPHERAL_ADDR, crumbs_arduino_write, &Wire);
    }
    else
    {
        Serial.print(F("Error: Unknown query opcode 0x"));
        Serial.println(query_op, HEX);
        return -1;
    }

    if (rc != 0)
    {
        Serial.print(F("Error: Failed to send query ("));
        Serial.print(rc);
        Serial.println(F(")"));
        return -1;
    }

    delay(10); /* Give peripheral time to process */

    /* Read reply */
    uint8_t raw[32];
    int n = crumbs_arduino_read(&ctx, PERIPHERAL_ADDR, raw, sizeof(raw), 10000);
    if (n < 0)
    {
        Serial.println(F("Error: No response from peripheral"));
        return -1;
    }

    /* Decode reply */
    crumbs_message_t reply;
    rc = crumbs_decode_message(raw, n, &reply, &ctx);
    if (rc != 0)
    {
        Serial.print(F("Error: Failed to decode reply ("));
        Serial.print(rc);
        Serial.println(F(")"));
        return -1;
    }

    /* Print reply */
    Serial.print(label);
    Serial.print(F(": "));

    if (query_op == MOCK_OP_GET_INFO || query_op == MOCK_OP_GET_ECHO)
    {
        /* Print as string/hex dump */
        for (uint8_t i = 0; i < reply.data_len; i++)
        {
            if (reply.data[i] >= 32 && reply.data[i] < 127)
            {
                Serial.write(reply.data[i]);
            }
            else
            {
                Serial.print(F("<0x"));
                if (reply.data[i] < 0x10)
                    Serial.print('0');
                Serial.print(reply.data[i], HEX);
                Serial.print(F(">"));
            }
        }
        Serial.println();

        /* Also print hex dump */
        if (reply.data_len > 0)
        {
            Serial.print(F("  Hex: "));
            for (uint8_t i = 0; i < reply.data_len; i++)
            {
                if (reply.data[i] < 0x10)
                    Serial.print('0');
                Serial.print(reply.data[i], HEX);
                Serial.print(' ');
            }
            Serial.println();
        }
    }
    else if (query_op == MOCK_OP_GET_STATUS)
    {
        /* Print status: state + heartbeat period */
        if (reply.data_len >= 3)
        {
            uint8_t state;
            uint16_t period;
            if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &state) == 0 &&
                crumbs_msg_read_u16(reply.data, reply.data_len, 1, &period) == 0)
            {
                Serial.print(F("Heartbeat: "));
                Serial.print(state ? F("ENABLED") : F("DISABLED"));
                Serial.print(F(", Period: "));
                Serial.print(period);
                Serial.println(F(" ms"));
            }
        }
        else
        {
            Serial.println(F("(invalid data)"));
        }
    }

    return 0;
}

/* ============================================================================
 * Command Handlers
 * ============================================================================ */

/**
 * @brief Handle 'echo' command.
 */
static void cmd_echo(const char *args)
{
    uint8_t data[27];
    uint8_t len = 0;

    /* Parse hex bytes from args */
    const char *p = args;
    while (*p && len < sizeof(data))
    {
        /* Skip whitespace */
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;

        /* Parse hex byte */
        char hex[3] = {0};
        if (*p && ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')))
        {
            hex[0] = *p++;
        }
        if (*p && ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f')))
        {
            hex[1] = *p++;
        }

        data[len++] = (uint8_t)strtol(hex, nullptr, 16);
    }

    if (len == 0)
    {
        Serial.println(F("Usage: echo <hex bytes> (e.g., 'echo DE AD BE EF')"));
        return;
    }

    /* Send using helper function */
    int rc = mock_send_echo(&ctx, PERIPHERAL_ADDR, crumbs_arduino_write, &Wire, data, len);
    if (rc == 0)
    {
        Serial.print(F("Sent echo data ("));
        Serial.print(len);
        Serial.println(F(" bytes)"));
    }
    else
    {
        Serial.println(F("Error: Failed to send echo"));
    }
}

/**
 * @brief Handle 'heartbeat' command.
 */
static void cmd_heartbeat(const char *args)
{
    /* Parse period in milliseconds */
    unsigned long period = strtoul(args, nullptr, 10);
    
    if (period > 65535)
    {
        Serial.println(F("Error: Period must be 0-65535 ms"));
        return;
    }

    if (*args == '\0')
    {
        Serial.println(F("Usage: heartbeat <ms> (e.g., 'heartbeat 500')"));
        return;
    }

    /* Send using helper function */
    int rc = mock_send_heartbeat(&ctx, PERIPHERAL_ADDR, crumbs_arduino_write, &Wire,
                                  (uint16_t)period);
    if (rc == 0)
    {
        Serial.print(F("Set heartbeat period to "));
        Serial.print(period);
        Serial.println(F(" ms"));
    }
    else
    {
        Serial.println(F("Error: Failed to send heartbeat command"));
}

/**
 * @brief Handle 'toggle' command.
 */
static void cmd_toggle()
{
    int rc = mock_send_toggle(&ctx, PERIPHERAL_ADDR, crumbs_arduino_write, &Wire);
    if (rc == 0)
    {
        Serial.println(F("Sent toggle command"));
    }
    else
    {
        Serial.println(F("Error: Failed to send toggle"));
    }
}

/**
 * @brief Handle 'scan' command.
 */
static void cmd_scan()
{
    Serial.println(F("Scanning I2C bus..."));

    uint8_t found = 0;
    for (uint8_t addr = 0x08; addr < 0x78; addr++)
    {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0)
        {
            Serial.print(F("  Device at 0x"));
            if (addr < 0x10)
                Serial.print('0');
            Serial.println(addr, HEX);
            found++;
        }
    }

    if (found == 0)
    {
        Serial.println(F("  No devices found"));
    }
}

/* ============================================================================
 * Command Parser
 * ============================================================================ */

/**
 * @brief Parse and execute command from buffer.
 */
static void parse_command()
{
    /* Trim trailing whitespace */
    while (cmd_len > 0 && (cmd_buffer[cmd_len - 1] == ' ' || cmd_buffer[cmd_len - 1] == '\r' || cmd_buffer[cmd_len - 1] == '\n'))
    {
        cmd_len--;
    }
    cmd_buffer[cmd_len] = '\0';

    if (cmd_len == 0)
        return;

    /* Find first space to separate command from args */
    char *args = strchr(cmd_buffer, ' ');
    if (args)
    {
        *args = '\0'; /* Terminate command */
        args++;       /* Point to arguments */

        /* Skip leading spaces in args */
        while (*args == ' ')
            args++;
    }
    else
    {
        args = cmd_buffer + cmd_len; /* Empty args */
    }

    /* Execute command */
    if (strcmp(cmd_buffer, "help") == 0)
    {
        print_help();
    }
    else if (strcmp(cmd_buffer, "echo") == 0)
    {
        cmd_echo(args);
    }
    else if (strcmp(cmd_buffer, "heartbeat") == 0)
    {
        cmd_heartbeat(args);
    }
    else if (strcmp(cmd_buffer, "toggle") == 0)
    {
        cmd_toggle();
    }
    else if (strcmp(cmd_buffer, "status") == 0)
    {
        query_and_print(MOCK_OP_GET_STATUS, "Status");
    }
    else if (strcmp(cmd_buffer, "getecho") == 0)
    {
        query_and_print(MOCK_OP_GET_ECHO, "Echo data");
    }
    else if (strcmp(cmd_buffer, "info") == 0)
    {
        query_and_print(MOCK_OP_GET_INFO, "Device info");
    }
    else if (strcmp(cmd_buffer, "scan") == 0)
    {
        cmd_scan();
    }
    else
    {
        Serial.print(F("Unknown command: "));
        Serial.println(cmd_buffer);
        Serial.println(F("Type 'help' for available commands"));
    }
}

/* ============================================================================
 * Setup & Loop
 * ============================================================================ */

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(1);
    }

    Serial.println(F("\n=== CRUMBS Mock Controller ==="));
    Serial.print(F("Target peripheral: 0x"));
    if (PERIPHERAL_ADDR < 0x10)
        Serial.print('0');
    Serial.println(PERIPHERAL_ADDR, HEX);

    /* Initialize CRUMBS context */
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    /* Initialize I2C as controller */
    Wire.begin();
    Wire.setClock(100000);

    print_help();
    Serial.print(F("> "));
}

void loop()
{
    /* Read serial input */
    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\n' || c == '\r')
        {
            Serial.println(); /* Echo newline */
            parse_command();
            cmd_len = 0;
            Serial.print(F("> "));
        }
        else if (c == '\b' || c == 0x7F) /* Backspace */
        {
            if (cmd_len > 0)
            {
                cmd_len--;
                Serial.write('\b');
                Serial.write(' ');
                Serial.write('\b');
            }
        }
        else if (cmd_len < MAX_CMD_LEN - 1)
        {
            cmd_buffer[cmd_len++] = c;
            Serial.write(c); /* Echo character */
        }
    }
}
