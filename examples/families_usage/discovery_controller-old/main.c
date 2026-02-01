/**
 * @file main.c
 * @brief Discovery-based interactive controller for lhwit_family peripherals.
 *
 * This controller demonstrates auto-discovery of CRUMBS devices by type.
 * It scans the I2C bus for devices, identifies them by type ID, and provides
 * an interactive command-line interface for sending commands to discovered devices.
 *
 * Supported Device Types:
 * - Calculator (Type 0x03): Math operations with history
 * - LED Array (Type 0x01): 4-LED control
 * - Servo Controller (Type 0x02): 2-servo position control
 *
 * Hardware Setup:
 * - Linux SBC (Raspberry Pi, etc.) as controller
 * - Calculator peripheral (Type 0x03, typically at 0x10)
 * - LED peripheral (Type 0x01, typically at 0x20)
 * - Servo peripheral (Type 0x02, typically at 0x30)
 *
 * Build:
 *   mkdir -p build && cd build
 *   cmake ..
 *   make
 *
 * Usage:
 *   ./lhwit_discovery_controller [i2c-device]
 *   ./lhwit_discovery_controller /dev/i2c-1
 *
 * Workflow:
 *   1. Run the 'scan' command to discover devices
 *   2. Use interactive commands to control devices
 *   3. Type 'help' for available commands
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crumbs.h"
#include "crumbs_linux.h"
#include "crumbs_message_helpers.h"

/* Include canonical operation headers */
#include "../lhwit_family/lhwit_ops.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

/* Discovered device addresses (0 = not found) */
static uint8_t g_calc_addr = 0;
static uint8_t g_led_addr = 0;
static uint8_t g_servo_addr = 0;

/* Maximum command line length */
#define MAX_CMD_LEN 256

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

static void print_help(void);
static void trim_whitespace(char *str);
static int cmd_scan(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw);
static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);

/* ============================================================================
 * Help Text
 * ============================================================================ */

static void print_help(void)
{
    printf("\n");
    printf("LHWIT Discovery Controller - Available Commands\n");
    printf("================================================\n");
    printf("\n");
    printf("General:\n");
    printf("  help                              - Show this help\n");
    printf("  scan                              - Scan I2C bus and identify devices by type\n");
    printf("  quit, exit                        - Exit the program\n");
    printf("\n");

    if (g_calc_addr)
    {
        printf("Calculator commands (0x%02X):\n", g_calc_addr);
    }
    else
    {
        printf("Calculator commands (not found - run 'scan'):\n");
    }
    printf("  calculator add <a> <b>            - Add two numbers\n");
    printf("  calculator sub <a> <b>            - Subtract (a - b)\n");
    printf("  calculator mul <a> <b>            - Multiply two numbers\n");
    printf("  calculator div <a> <b>            - Divide (a / b)\n");
    printf("  calculator result                 - Get last result\n");
    printf("  calculator history                - Get operation history\n");
    printf("\n");

    if (g_led_addr)
    {
        printf("LED commands (0x%02X):\n", g_led_addr);
    }
    else
    {
        printf("LED commands (not found - run 'scan'):\n");
    }
    printf("  led set_all <mask>                - Set all LEDs (e.g., 0x0F)\n");
    printf("  led set_one <idx> <0|1>           - Set single LED (idx 0-3)\n");
    printf("  led blink <idx> <en> <period>     - Blink LED (period in ms)\n");
    printf("  led get_state                     - Get current LED state\n");
    printf("\n");

    if (g_servo_addr)
    {
        printf("Servo commands (0x%02X):\n", g_servo_addr);
    }
    else
    {
        printf("Servo commands (not found - run 'scan'):\n");
    }
    printf("  servo set_pos <idx> <angle>       - Set position (idx 0-1, angle 0-180)\n");
    printf("  servo set_speed <idx> <speed>     - Set speed (0=instant, 1-20=slow)\n");
    printf("  servo sweep <idx> <en> <min> <max> <step> - Configure sweep\n");
    printf("  servo get_pos                     - Get current positions\n");
    printf("\n");
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Trim leading and trailing whitespace from a string in-place.
 */
static void trim_whitespace(char *str)
{
    if (!str || !*str)
        return;

    /* Trim leading */
    char *start = str;
    while (isspace((unsigned char)*start))
        start++;

    /* Trim trailing */
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
        end--;
    end[1] = '\0';

    /* Shift if needed */
    if (start != str)
        memmove(str, start, strlen(start) + 1);
}

/* ============================================================================
 * Command: scan
 * ============================================================================ */

static int cmd_scan(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw)
{
    printf("Scanning I2C bus for CRUMBS devices (0x08-0x77)...\n");

    /* Arrays to store discovered devices */
    uint8_t types[16];
    uint8_t addrs[16];

    /* Scan with type identification */
    int count = crumbs_controller_scan_for_crumbs_with_types(
        ctx, 0x08, 0x77,
        crumbs_linux_i2c_write, crumbs_linux_read, lw,
        types, addrs, 16);

    if (count < 0)
    {
        fprintf(stderr, "  ERROR: Scan failed (%d)\n", count);
        return -1;
    }

    if (count == 0)
    {
        printf("  No CRUMBS devices found.\n");
        printf("  Make sure peripherals are powered and connected.\n");
        return 0;
    }

    /* Reset discovered addresses */
    g_calc_addr = 0;
    g_led_addr = 0;
    g_servo_addr = 0;

    /* Identify and store device addresses by type */
    printf("  Found %d device(s):\n", count);
    for (int i = 0; i < count; i++)
    {
        printf("    0x%02X: Type 0x%02X", addrs[i], types[i]);

        switch (types[i])
        {
        case CALC_TYPE_ID:
            printf(" - Calculator");
            g_calc_addr = addrs[i];
            break;
        case LED_TYPE_ID:
            printf(" - LED Array");
            g_led_addr = addrs[i];
            break;
        case SERVO_TYPE_ID:
            printf(" - Servo Controller");
            g_servo_addr = addrs[i];
            break;
        default:
            printf(" - Unknown");
            break;
        }
        printf("\n");
    }

    printf("\n");
    printf("Device addresses stored. You can now use device commands.\n");
    printf("Type 'help' to see available commands.\n");

    return 0;
}

/* ============================================================================
 * Command: calculator
 * ============================================================================ */

static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    if (g_calc_addr == 0)
    {
        printf("ERROR: Calculator not found. Run 'scan' first.\n");
        return -1;
    }

    char subcmd[32] = {0};
    if (!args || sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: calculator <add|sub|mul|div|result|history> [args...]\n");
        return -1;
    }

    /* Skip past the subcmd in args */
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;
    crumbs_message_t msg, reply;

    if (strcmp(subcmd, "add") == 0 || strcmp(subcmd, "sub") == 0 ||
        strcmp(subcmd, "mul") == 0 || strcmp(subcmd, "div") == 0)
    {
        uint32_t a, b;
        if (sscanf(rest, "%u %u", &a, &b) != 2)
        {
            printf("Usage: calculator %s <a> <b>\n", subcmd);
            return -1;
        }

        /* Build and send command */
        if (strcmp(subcmd, "add") == 0)
            rc = calc_send_add(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);
        else if (strcmp(subcmd, "sub") == 0)
            rc = calc_send_sub(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);
        else if (strcmp(subcmd, "mul") == 0)
            rc = calc_send_mul(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);
        else
            rc = calc_send_div(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);

        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to send operation (%d)\n", rc);
            return rc;
        }

        printf("OK: %s(%u, %u) command sent\n", subcmd, a, b);
        printf("    Use 'calculator result' to get the answer\n");
        return 0;
    }
    else if (strcmp(subcmd, "result") == 0)
    {
        /* Send GET_RESULT request */
        rc = calc_query_result(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to request result (%d)\n", rc);
            return rc;
        }

        /* Read reply */
        rc = crumbs_linux_read_message(lw, g_calc_addr, ctx, &reply);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to read result (%d)\n", rc);
            return rc;
        }

        uint32_t result;
        if (crumbs_msg_read_u32(reply.data, reply.data_len, 0, &result) == 0)
        {
            printf("Last result: %u\n", result);
        }
        else
        {
            fprintf(stderr, "ERROR: Invalid response format\n");
            return -1;
        }
        return 0;
    }
    else if (strcmp(subcmd, "history") == 0)
    {
        /* First, get history metadata */
        rc = calc_query_hist_meta(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to request history metadata (%d)\n", rc);
            return rc;
        }

        rc = crumbs_linux_read_message(lw, g_calc_addr, ctx, &reply);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to read metadata (%d)\n", rc);
            return rc;
        }

        uint8_t count, write_pos;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &count) != 0 ||
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &write_pos) != 0)
        {
            fprintf(stderr, "ERROR: Invalid metadata format\n");
            return -1;
        }

        printf("History: %u entries (write position: %u)\n", count, write_pos);

        if (count == 0)
        {
            printf("  (no operations recorded yet)\n");
            return 0;
        }

        /* Retrieve each history entry */
        for (uint8_t i = 0; i < count; i++)
        {
            rc = calc_query_hist_entry(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, i);
            if (rc != 0)
            {
                fprintf(stderr, "ERROR: Failed to request entry %u (%d)\n", i, rc);
                continue;
            }

            rc = crumbs_linux_read_message(lw, g_calc_addr, ctx, &reply);
            if (rc != 0)
            {
                fprintf(stderr, "ERROR: Failed to read entry %u (%d)\n", i, rc);
                continue;
            }

            if (reply.data_len >= 16)
            {
                char op[5] = {0};
                memcpy(op, reply.data, 4);

                uint32_t a, b, result;
                crumbs_msg_read_u32(reply.data, reply.data_len, 4, &a);
                crumbs_msg_read_u32(reply.data, reply.data_len, 8, &b);
                crumbs_msg_read_u32(reply.data, reply.data_len, 12, &result);

                printf("  [%u] %s(%u, %u) = %u\n", i, op, a, b, result);
            }
        }
        return 0;
    }
    else
    {
        printf("Unknown calculator command: %s\n", subcmd);
        printf("Available: add, sub, mul, div, result, history\n");
        return -1;
    }
}

/* ============================================================================
 * Command: led
 * ============================================================================ */

static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    if (g_led_addr == 0)
    {
        printf("ERROR: LED array not found. Run 'scan' first.\n");
        return -1;
    }

    char subcmd[32] = {0};
    if (!args || sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: led <set_all|set_one|blink|get_state> [args...]\n");
        return -1;
    }

    /* Skip past the subcmd in args */
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;
    crumbs_message_t reply;

    if (strcmp(subcmd, "set_all") == 0)
    {
        unsigned int mask;
        if (sscanf(rest, "%i", (int *)&mask) != 1 || mask > 0xFF)
        {
            printf("Usage: led set_all <mask>  (e.g., 0x0F or 15)\n");
            return -1;
        }

        rc = led_send_set_all(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw, (uint8_t)mask);
        if (rc == 0)
            printf("OK: All LEDs set to 0x%02X\n", (uint8_t)mask);
        else
            fprintf(stderr, "ERROR: led_send_set_all failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "set_one") == 0)
    {
        unsigned int idx, state;
        if (sscanf(rest, "%u %u", &idx, &state) != 2 || idx > 3 || state > 1)
        {
            printf("Usage: led set_one <idx> <0|1>  (idx 0-3)\n");
            return -1;
        }

        rc = led_send_set_one(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw, 
                              (uint8_t)idx, (uint8_t)state);
        if (rc == 0)
            printf("OK: LED %u set to %s\n", idx, state ? "ON" : "OFF");
        else
            fprintf(stderr, "ERROR: led_send_set_one failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "blink") == 0)
    {
        unsigned int idx, enable, period_ms;
        if (sscanf(rest, "%u %u %u", &idx, &enable, &period_ms) != 3 ||
            idx > 3 || enable > 1 || period_ms > 65535)
        {
            printf("Usage: led blink <idx> <0|1> <period_ms>  (idx 0-3)\n");
            return -1;
        }

        rc = led_send_blink(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw,
                            (uint8_t)idx, (uint8_t)enable, (uint16_t)period_ms);
        if (rc == 0)
        {
            if (enable)
                printf("OK: LED %u blinking with %u ms period\n", idx, period_ms);
            else
                printf("OK: LED %u blink disabled\n", idx);
        }
        else
            fprintf(stderr, "ERROR: led_send_blink failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "get_state") == 0)
    {
        rc = led_query_state(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: led_send_get_state failed (%d)\n", rc);
            return rc;
        }

        rc = crumbs_linux_read_message(lw, g_led_addr, ctx, &reply);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to read LED state (%d)\n", rc);
            return rc;
        }

        uint8_t state;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &state) == 0)
        {
            printf("LED state: 0x%02X (", state);
            for (int i = 3; i >= 0; i--)
            {
                printf("%d", (state >> i) & 1);
            }
            printf(")\n");
            printf("  LED 0: %s, LED 1: %s, LED 2: %s, LED 3: %s\n",
                   (state & 0x01) ? "ON" : "OFF",
                   (state & 0x02) ? "ON" : "OFF",
                   (state & 0x04) ? "ON" : "OFF",
                   (state & 0x08) ? "ON" : "OFF");
        }
        else
        {
            fprintf(stderr, "ERROR: Invalid response format\n");
            return -1;
        }
        return 0;
    }
    else
    {
        printf("Unknown LED command: %s\n", subcmd);
        printf("Available: set_all, set_one, blink, get_state\n");
        return -1;
    }
}

/* ============================================================================
 * Command: servo
 * ============================================================================ */

static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    if (g_servo_addr == 0)
    {
        printf("ERROR: Servo controller not found. Run 'scan' first.\n");
        return -1;
    }

    char subcmd[32] = {0};
    if (!args || sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: servo <set_pos|set_speed|sweep|get_pos> [args...]\n");
        return -1;
    }

    /* Skip past the subcmd in args */
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;
    crumbs_message_t reply;

    if (strcmp(subcmd, "set_pos") == 0)
    {
        unsigned int idx, position;
        if (sscanf(rest, "%u %u", &idx, &position) != 2 || idx > 1 || position > 180)
        {
            printf("Usage: servo set_pos <idx> <angle>  (idx 0-1, angle 0-180)\n");
            return -1;
        }

        rc = servo_send_set_pos(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw,
                                (uint8_t)idx, (uint8_t)position);
        if (rc == 0)
            printf("OK: Servo %u set to %u degrees\n", idx, position);
        else
            fprintf(stderr, "ERROR: servo_send_set_pos failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "set_speed") == 0)
    {
        unsigned int idx, speed;
        if (sscanf(rest, "%u %u", &idx, &speed) != 2 || idx > 1 || speed > 20)
        {
            printf("Usage: servo set_speed <idx> <speed>  (idx 0-1, speed 0-20)\n");
            return -1;
        }

        rc = servo_send_set_speed(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw,
                                  (uint8_t)idx, (uint8_t)speed);
        if (rc == 0)
            printf("OK: Servo %u speed set to %u (0=instant, higher=slower)\n", idx, speed);
        else
            fprintf(stderr, "ERROR: servo_send_set_speed failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "sweep") == 0)
    {
        unsigned int idx, enable, min_pos, max_pos, step;
        if (sscanf(rest, "%u %u %u %u %u", &idx, &enable, &min_pos, &max_pos, &step) != 5 ||
            idx > 1 || enable > 1 || min_pos > 180 || max_pos > 180 || step > 255)
        {
            printf("Usage: servo sweep <idx> <0|1> <min> <max> <step>\n");
            printf("       idx: 0-1, min/max: 0-180, step: degrees per update\n");
            return -1;
        }

        rc = servo_send_sweep(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw,
                              (uint8_t)idx, (uint8_t)enable, (uint8_t)min_pos,
                              (uint8_t)max_pos, (uint8_t)step);
        if (rc == 0)
        {
            if (enable)
                printf("OK: Servo %u sweeping from %u to %u degrees (step %u)\n",
                       idx, min_pos, max_pos, step);
            else
                printf("OK: Servo %u sweep disabled\n", idx);
        }
        else
            fprintf(stderr, "ERROR: servo_send_sweep failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "get_pos") == 0)
    {
        rc = servo_query_pos(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: servo_send_get_pos failed (%d)\n", rc);
            return rc;
        }

        rc = crumbs_linux_read_message(lw, g_servo_addr, ctx, &reply);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to read servo positions (%d)\n", rc);
            return rc;
        }

        uint8_t pos0, pos1;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &pos0) == 0 &&
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &pos1) == 0)
        {
            printf("Servo positions: Servo 0 = %u°, Servo 1 = %u°\n", pos0, pos1);
        }
        else
        {
            fprintf(stderr, "ERROR: Invalid response format\n");
            return -1;
        }
        return 0;
    }
    else
    {
        printf("Unknown servo command: %s\n", subcmd);
        printf("Available: set_pos, set_speed, sweep, get_pos\n");
        return -1;
    }
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char **argv)
{
    printf("\n");
    printf("LHWIT Discovery Controller\n");
    printf("==========================\n");
    printf("\n");

    crumbs_context_t ctx;
    crumbs_linux_i2c_t lw;

    const char *device_path = "/dev/i2c-1";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0')
        device_path = argv[1];

    printf("I2C Device: %s\n", device_path);
    printf("\n");
    printf("Getting started:\n");
    printf("  1. Run 'scan' to discover devices\n");
    printf("  2. Use device commands (calculator, led, servo)\n");
    printf("  3. Type 'help' for command reference\n");
    printf("\n");

    /* Initialize controller */
    int rc = crumbs_linux_init_controller(&ctx, &lw, device_path, 25000);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR: Failed to initialize I2C controller (%d)\n", rc);
        fprintf(stderr, "Make sure:\n");
        fprintf(stderr, "  - I2C device exists (%s)\n", device_path);
        fprintf(stderr, "  - You have permissions (try: sudo chmod 666 %s)\n", device_path);
        fprintf(stderr, "  - I2C is enabled on your system\n");
        return 1;
    }

    /* Command loop */
    char line[MAX_CMD_LEN];
    while (1)
    {
        printf("lhwit> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
        {
            /* EOF or error */
            printf("\n");
            break;
        }

        /* Remove newline and trim whitespace */
        line[strcspn(line, "\r\n")] = '\0';
        trim_whitespace(line);

        /* Skip empty lines */
        if (!line[0])
            continue;

        /* Parse command */
        char cmd[32] = {0};
        if (sscanf(line, "%31s", cmd) != 1)
            continue;

        /* Find start of arguments (after command) */
        const char *args = line + strlen(cmd);
        while (*args && isspace((unsigned char)*args))
            args++;

        /* Dispatch */
        if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0)
        {
            print_help();
        }
        else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0)
        {
            printf("Goodbye!\n");
            break;
        }
        else if (strcmp(cmd, "scan") == 0)
        {
            cmd_scan(&ctx, &lw);
        }
        else if (strcmp(cmd, "calculator") == 0 || strcmp(cmd, "calc") == 0)
        {
            cmd_calculator(&ctx, &lw, args);
        }
        else if (strcmp(cmd, "led") == 0)
        {
            cmd_led(&ctx, &lw, args);
        }
        else if (strcmp(cmd, "servo") == 0)
        {
            cmd_servo(&ctx, &lw, args);
        }
        else
        {
            printf("Unknown command: %s\n", cmd);
            printf("Type 'help' for available commands.\n");
        }
    }

    /* Cleanup */
    crumbs_linux_close(&lw);
    return 0;
}
