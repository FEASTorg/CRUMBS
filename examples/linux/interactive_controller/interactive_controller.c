/**
 * @file interactive_controller.c
 * @brief Interactive Linux controller for LED and servo test peripherals.
 *
 * This example provides a command-line interface to control LED and servo
 * peripherals interactively. Instead of running a hardcoded demo sequence,
 * users can type commands to control devices in real-time.
 *
 * Hardware Setup:
 * - Linux SBC (Raspberry Pi, etc.) as controller
 * - LED peripheral at I2C address 0x08 (default, configurable)
 * - Servo peripheral at I2C address 0x09 (default, configurable)
 *
 * Build (in-tree):
 *   mkdir -p build && cd build
 *   cmake .. -DCRUMBS_ENABLE_LINUX_HAL=ON -DCRUMBS_BUILD_EXAMPLES=ON
 *   cmake --build . --parallel
 *
 * Usage:
 *   ./crumbs_interactive_controller [i2c-device]
 *   ./crumbs_interactive_controller /dev/i2c-1
 *
 * Type 'help' at the prompt for available commands.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

#define ENABLE_CRUMBS_DEBUG 0

#if ENABLE_CRUMBS_DEBUG
#define CRUMBS_DEBUG
#define CRUMBS_DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#endif

#include "crumbs.h"
#include "crumbs_linux.h"
#include "crumbs_message_helpers.h"

/* Command definitions */
#include "led_commands.h"
#include "servo_commands.h"

/* ============================================================================
 * Configuration (modifiable at runtime)
 * ============================================================================ */

static uint8_t g_led_addr = 0x08;
static uint8_t g_servo_addr = 0x09;

/* Maximum command line length */
#define MAX_CMD_LEN 256

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

static void print_help(void);
static void print_led_state_binary(uint8_t state);
static int cmd_scan(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw);
static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_addr(const char *args);
static void trim_whitespace(char *str);

/* ============================================================================
 * Help Text
 * ============================================================================ */

static void print_help(void)
{
    printf("\n");
    printf("Available commands:\n");
    printf("  help                          - Show this help\n");
    printf("  scan                          - Scan I2C bus for CRUMBS devices\n");
    printf("  quit, exit                    - Exit the program\n");
    printf("\n");
    printf("LED commands (target: 0x%02X):\n", g_led_addr);
    printf("  led set_all <bitmask>         - Set all LEDs (e.g., 'led set_all 0x0F')\n");
    printf("  led set <index> <0|1>         - Set single LED on/off (index 0-7)\n");
    printf("  led blink <count> <delay>     - Blink all LEDs (delay in 10ms units)\n");
    printf("  led state                     - Get current LED state\n");
    printf("\n");
    printf("Servo commands (target: 0x%02X):\n", g_servo_addr);
    printf("  servo angle <ch> <deg>        - Set servo angle (ch 0-1, deg 0-180)\n");
    printf("  servo both <deg0> <deg1>      - Set both servo angles\n");
    printf("  servo sweep <ch> <s> <e> <ms> - Sweep servo (start to end, ms/step)\n");
    printf("  servo center                  - Center all servos to 90 degrees\n");
    printf("  servo angles                  - Get current servo angles\n");
    printf("\n");
    printf("Address commands:\n");
    printf("  addr led <hex>                - Set LED peripheral address\n");
    printf("  addr servo <hex>              - Set servo peripheral address\n");
    printf("  addr                          - Show current addresses\n");
    printf("\n");
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Print LED state as binary representation.
 */
static void print_led_state_binary(uint8_t state)
{
    printf("LED state: 0x%02X (", state);
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (state >> i) & 1);
        if (i == 4)
            printf(" "); /* Visual separator */
    }
    printf(")\n");
}

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
    printf("Scanning for CRUMBS devices (0x03-0x77)...\n");

    uint8_t found[128];
    int n = crumbs_controller_scan_for_crumbs(
        ctx, 0x03, 0x77, 0, /* non-strict */
        crumbs_linux_i2c_write, crumbs_linux_read, lw,
        found, sizeof(found), 25000);

    if (n < 0)
    {
        fprintf(stderr, "  ERROR: scan failed (%d)\n", n);
        return -1;
    }

    if (n == 0)
    {
        printf("  No CRUMBS devices found.\n");
    }
    else
    {
        printf("  Found %d device(s):\n", n);
        for (int i = 0; i < n; i++)
        {
            printf("    0x%02X", found[i]);
            if (found[i] == g_led_addr)
                printf(" (LED)");
            if (found[i] == g_servo_addr)
                printf(" (Servo)");
            printf("\n");
        }
    }
    return 0;
}

/* ============================================================================
 * Command: led
 * ============================================================================ */

static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    char subcmd[32] = {0};
    int rc;

    if (!args || sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: led <set_all|set|blink|state> [args...]\n");
        return -1;
    }

    /* Skip past the subcmd in args */
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    if (strcmp(subcmd, "set_all") == 0)
    {
        unsigned int bitmask;
        if (sscanf(rest, "%i", (int *)&bitmask) != 1 || bitmask > 0xFF)
        {
            printf("Usage: led set_all <bitmask>  (e.g., 0x0F or 15)\n");
            return -1;
        }
        rc = led_send_set_all(ctx, g_led_addr, crumbs_linux_i2c_write, lw,
                              (uint8_t)bitmask);
        if (rc == 0)
            printf("OK: Set all LEDs to 0x%02X\n", (uint8_t)bitmask);
        else
            fprintf(stderr, "ERROR: led_send_set_all failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "set") == 0)
    {
        unsigned int index, state;
        if (sscanf(rest, "%u %u", &index, &state) != 2 || index > 7 || state > 1)
        {
            printf("Usage: led set <index> <0|1>  (index 0-7)\n");
            return -1;
        }
        rc = led_send_set_one(ctx, g_led_addr, crumbs_linux_i2c_write, lw,
                              (uint8_t)index, (uint8_t)state);
        if (rc == 0)
            printf("OK: LED %u set to %s\n", index, state ? "ON" : "OFF");
        else
            fprintf(stderr, "ERROR: led_send_set_one failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "blink") == 0)
    {
        unsigned int count, delay;
        if (sscanf(rest, "%u %u", &count, &delay) != 2 || count > 255 || delay > 255)
        {
            printf("Usage: led blink <count> <delay_10ms>  (both 0-255)\n");
            return -1;
        }
        rc = led_send_blink(ctx, g_led_addr, crumbs_linux_i2c_write, lw,
                            (uint8_t)count, (uint8_t)delay);
        if (rc == 0)
            printf("OK: Blinking %u times with %u*10ms delay\n", count, delay);
        else
            fprintf(stderr, "ERROR: led_send_blink failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "state") == 0)
    {
        /* Send GET_STATE command */
        rc = led_send_get_state(ctx, g_led_addr, crumbs_linux_i2c_write, lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: led_send_get_state failed (%d)\n", rc);
            return rc;
        }

        /* Read response */
        crumbs_message_t reply;
        rc = crumbs_linux_read_message(lw, g_led_addr, ctx, &reply);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to read LED state (%d)\n", rc);
            return rc;
        }

        uint8_t state;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &state) == 0)
        {
            print_led_state_binary(state);
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
        printf("Available: set_all, set, blink, state\n");
        return -1;
    }
}

/* ============================================================================
 * Command: servo
 * ============================================================================ */

static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    char subcmd[32] = {0};
    int rc;

    if (!args || sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: servo <angle|both|sweep|center|angles> [args...]\n");
        return -1;
    }

    /* Skip past the subcmd in args */
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    if (strcmp(subcmd, "angle") == 0)
    {
        unsigned int channel, angle;
        if (sscanf(rest, "%u %u", &channel, &angle) != 2 || channel > 1 || angle > 180)
        {
            printf("Usage: servo angle <channel> <degrees>  (ch 0-1, deg 0-180)\n");
            return -1;
        }
        rc = servo_send_angle(ctx, g_servo_addr, crumbs_linux_i2c_write, lw,
                              (uint8_t)channel, (uint8_t)angle);
        if (rc == 0)
            printf("OK: Servo %u set to %u degrees\n", channel, angle);
        else
            fprintf(stderr, "ERROR: servo_send_angle failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "both") == 0)
    {
        unsigned int angle0, angle1;
        if (sscanf(rest, "%u %u", &angle0, &angle1) != 2 || angle0 > 180 || angle1 > 180)
        {
            printf("Usage: servo both <angle0> <angle1>  (both 0-180)\n");
            return -1;
        }
        rc = servo_send_both(ctx, g_servo_addr, crumbs_linux_i2c_write, lw,
                             (uint8_t)angle0, (uint8_t)angle1);
        if (rc == 0)
            printf("OK: Servos set to %u, %u degrees\n", angle0, angle1);
        else
            fprintf(stderr, "ERROR: servo_send_both failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "sweep") == 0)
    {
        unsigned int channel, start, end, step_ms;
        if (sscanf(rest, "%u %u %u %u", &channel, &start, &end, &step_ms) != 4 ||
            channel > 1 || start > 180 || end > 180 || step_ms > 255)
        {
            printf("Usage: servo sweep <ch> <start> <end> <step_ms>\n");
            printf("       ch: 0-1, angles: 0-180, step_ms: 0-255\n");
            return -1;
        }
        rc = servo_send_sweep(ctx, g_servo_addr, crumbs_linux_i2c_write, lw,
                              (uint8_t)channel, (uint8_t)start, (uint8_t)end, (uint8_t)step_ms);
        if (rc == 0)
            printf("OK: Sweeping servo %u from %u to %u (%u ms/step)\n",
                   channel, start, end, step_ms);
        else
            fprintf(stderr, "ERROR: servo_send_sweep failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "center") == 0)
    {
        rc = servo_send_center_all(ctx, g_servo_addr, crumbs_linux_i2c_write, lw);
        if (rc == 0)
            printf("OK: All servos centered to 90 degrees\n");
        else
            fprintf(stderr, "ERROR: servo_send_center_all failed (%d)\n", rc);
        return rc;
    }
    else if (strcmp(subcmd, "angles") == 0)
    {
        /* Send GET_ANGLES command */
        rc = servo_send_get_angles(ctx, g_servo_addr, crumbs_linux_i2c_write, lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: servo_send_get_angles failed (%d)\n", rc);
            return rc;
        }

        /* Read response */
        crumbs_message_t reply;
        rc = crumbs_linux_read_message(lw, g_servo_addr, ctx, &reply);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to read servo angles (%d)\n", rc);
            return rc;
        }

        uint8_t angle0, angle1;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &angle0) == 0 &&
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &angle1) == 0)
        {
            printf("Servo angles: ch0=%u deg, ch1=%u deg\n", angle0, angle1);
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
        printf("Available: angle, both, sweep, center, angles\n");
        return -1;
    }
}

/* ============================================================================
 * Command: addr
 * ============================================================================ */

static int cmd_addr(const char *args)
{
    char device[32] = {0};
    unsigned int addr;

    if (!args || !*args)
    {
        /* Show current addresses */
        printf("Current addresses:\n");
        printf("  LED:   0x%02X\n", g_led_addr);
        printf("  Servo: 0x%02X\n", g_servo_addr);
        return 0;
    }

    if (sscanf(args, "%31s %i", device, (int *)&addr) != 2 || addr > 0x7F)
    {
        printf("Usage: addr <led|servo> <hex_address>\n");
        printf("       addr               (show current addresses)\n");
        return -1;
    }

    if (strcmp(device, "led") == 0)
    {
        g_led_addr = (uint8_t)addr;
        printf("OK: LED address set to 0x%02X\n", g_led_addr);
    }
    else if (strcmp(device, "servo") == 0)
    {
        g_servo_addr = (uint8_t)addr;
        printf("OK: Servo address set to 0x%02X\n", g_servo_addr);
    }
    else
    {
        printf("Unknown device: %s (use 'led' or 'servo')\n", device);
        return -1;
    }
    return 0;
}

/* ============================================================================
 * Main Loop
 * ============================================================================ */

int main(int argc, char **argv)
{
    printf("CRUMBS Interactive Controller\n");
    printf("==============================\n");

    crumbs_context_t ctx;
    crumbs_linux_i2c_t lw;

    /* Get I2C device path from command line */
    const char *device_path = "/dev/i2c-1";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0')
    {
        device_path = argv[1];
    }

    printf("I2C Device: %s\n", device_path);
    printf("Default LED address: 0x%02X\n", g_led_addr);
    printf("Default Servo address: 0x%02X\n", g_servo_addr);
    printf("Type 'help' for available commands.\n\n");

    /* Initialize controller */
    int rc = crumbs_linux_init_controller(&ctx, &lw, device_path, 25000);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR: crumbs_linux_init_controller failed (%d)\n", rc);
        fprintf(stderr, "Make sure I2C device exists and you have permissions.\n");
        return 1;
    }

    /* Command loop */
    char line[MAX_CMD_LEN];
    while (1)
    {
        printf("crumbs> ");
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
        else if (strcmp(cmd, "led") == 0)
        {
            cmd_led(&ctx, &lw, args);
        }
        else if (strcmp(cmd, "servo") == 0)
        {
            cmd_servo(&ctx, &lw, args);
        }
        else if (strcmp(cmd, "addr") == 0)
        {
            cmd_addr(args);
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
