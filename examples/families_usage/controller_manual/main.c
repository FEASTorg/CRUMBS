/**
 * @file main.c
 * @brief Manual (preconfigured) controller for lhwit_family peripherals.
 *
 * This is a complete interactive application demonstrating standard CRUMBS usage:
 * - Preconfigured device list from config.h (typical production pattern)
 * - Supports multiple devices of same type
 * - Command sending via canonical *_ops.h helper functions
 * - SET_REPLY query pattern (write query opcode, then read response)
 * - Platform-specific reading via crumbs_linux_read_message()
 *
 * Structure:
 * - Configuration: Device array in config.h {type_id, address}
 * - Commands: Use canonical helper functions from *_ops.h
 * - Queries: Use SET_REPLY pattern (2-step: query write + response read)
 * - UI: Interactive shell (application layer, not library code)
 *
 * Difference from controller_discovery:
 * - No scan command (addresses are preconfigured)
 * - Faster startup (no bus scan required)
 * - Deterministic device order
 *
 * Usage:
 *   ./controller_manual [/dev/i2c-1]
 *   > list
 *   > calculator 0 add 10 20
 *   > led 0 set_all 0x0F
 *   > servo 0 set_pos 0 90
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crumbs.h"
#include "crumbs_linux.h"
#include "crumbs_message_helpers.h"

#include "../lhwit_family/lhwit_ops.h"
#include "config.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

/* Device info with index for addressing */
typedef struct
{
    uint8_t type_id;
    uint8_t addr;
    const char *name;
    int index; /* Index among devices of same type */
} device_info_t;

static device_info_t g_devices[16];
static int g_device_count = 0;

#define MAX_CMD_LEN 256

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

static void print_help(void);
static void cmd_list(void);
static void trim_whitespace(char *str);
static int find_device(uint8_t type_id, int index, uint8_t *addr_out);
static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_display(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);

/* ============================================================================
 * Help
 * ============================================================================ */

static void print_help(void)
{
    printf("\nLHWIT Manual Controller Commands\n");
    printf("=================================\n\n");
    printf("General:\n");
    printf("  help                              - Show this help\n");
    printf("  list                              - List configured devices\n");
    printf("  quit, exit                        - Exit\n\n");

    printf("Device Selection:\n");
    printf("  <type> <idx> <cmd> [args]         - Command to device by index\n");
    printf("  <type> @<addr> <cmd> [args]       - Command to device by address\n\n");

    printf("Calculator:\n");
    printf("  calculator 0 add <a> <b>          - Add\n");
    printf("  calculator 0 sub <a> <b>          - Subtract\n");
    printf("  calculator 0 mul <a> <b>          - Multiply\n");
    printf("  calculator 0 div <a> <b>          - Divide\n");
    printf("  calculator 0 result               - Get last result\n");
    printf("  calculator 0 history              - Show history\n\n");

    printf("LED:\n");
    printf("  led 0 set_all <mask>              - Set all LEDs (0x0F = all on)\n");
    printf("  led 0 set_one <idx> <0|1>         - Set single LED\n");
    printf("  led 0 blink <idx> <en> <ms>       - Configure blink\n");
    printf("  led 0 get_state                   - Get LED state\n\n");

    printf("Servo:\n");
    printf("  servo 0 set_pos <idx> <angle>     - Set position (0-180deg)\n");
    printf("  servo 0 set_speed <idx> <speed>   - Set speed (0-20)\n");
    printf("  servo 0 sweep <i> <en> <min> <max> <step> - Configure sweep\n");
    printf("  servo 0 get_pos                   - Get positions\n\n");

    printf("Display:\n");
    printf("  display 0 set_number <num> <dec>  - Display number (dec=0-4)\n");
    printf("  display 0 set_brightness <level>  - Set brightness (0-10)\n");
    printf("  display 0 clear                   - Clear display\n");
    printf("  display 0 get_value               - Get current value\n\n");
}
/* ============================================================================
 * List Command
 * ============================================================================ */

static void cmd_list(void)
{
    if (g_device_count == 0)
    {
        printf("No devices configured.\n");
        return;
    }

    printf("\nConfigured Devices:\n");
    printf("-------------------\n");
    for (int i = 0; i < g_device_count; i++)
    {
        printf("[%d] %s at 0x%02X (Type 0x%02X, Index %d)\n",
               i, g_devices[i].name, g_devices[i].addr,
               g_devices[i].type_id, g_devices[i].index);
    }
    printf("\n");
}

/* ============================================================================
 * Device Lookup
 * ============================================================================ */

/**
 * @brief Find device address by type and index among devices of that type.
 * @param type_id  Device type ID
 * @param index    Index among devices of same type (0-based)
 * @param addr_out Output address
 * @return 0 on success, -1 if not found
 */
static int find_device(uint8_t type_id, int index, uint8_t *addr_out)
{
    int type_count = 0;
    for (int i = 0; i < g_device_count; i++)
    {
        if (g_devices[i].type_id == type_id)
        {
            if (type_count == index)
            {
                *addr_out = g_devices[i].addr;
                return 0;
            }
            type_count++;
        }
    }
    return -1;
}
static void trim_whitespace(char *str)
{
    if (!str)
        return;
    char *start = str;
    while (*start && isspace((unsigned char)*start))
        start++;
    if (*start == '\0')
    {
        str[0] = '\0';
        return;
    }
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
        end--;
    size_t len = (size_t)(end - start + 1);
    memmove(str, start, len);
    str[len] = '\0';
}

/* ============================================================================
 * Calculator Commands
 * ============================================================================ */

static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    /* Parse device selector: either index or @address */
    uint8_t addr = 0;
    const char *cmd_start = args;

    if (*args == '@')
    {
        /* Address selector: @0x10 */
        unsigned int addr_val;
        if (sscanf(args, "@%i", &addr_val) != 1)
        {
            printf("Usage: calculator @<addr> <cmd> or calculator <idx> <cmd>\\n");
            return -1;
        }
        addr = (uint8_t)addr_val;
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command after address\\n");
            return -1;
        }
        cmd_start++;
    }
    else
    {
        /* Index selector: 0, 1, 2... */
        int idx;
        if (sscanf(args, "%d", &idx) != 1)
        {
            printf("Usage: calculator <idx> <cmd> or calculator @<addr> <cmd>\\n");
            return -1;
        }
        if (find_device(CALC_TYPE_ID, idx, &addr) != 0)
        {
            printf("Calculator #%d not found (run 'list' to see devices)\\n", idx);
            return -1;
        }
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command after device index\\n");
            return -1;
        }
        cmd_start++;
    }

    while (*cmd_start && isspace((unsigned char)*cmd_start))
        cmd_start++;

    char subcmd[32];
    if (sscanf(cmd_start, "%31s", subcmd) != 1)
    {
        printf("Usage: calculator <idx|@addr> <add|sub|mul|div|result|history> [args...]\\n");
        return -1;
    }

    const char *rest = cmd_start + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;
    crumbs_message_t reply;

    if (strcmp(subcmd, "add") == 0 || strcmp(subcmd, "sub") == 0 ||
        strcmp(subcmd, "mul") == 0 || strcmp(subcmd, "div") == 0)
    {
        uint32_t a, b;
        if (sscanf(rest, "%u %u", &a, &b) != 2)
        {
            printf("Usage: calculator <idx|@addr> %s <a> <b>\n", subcmd);
            return -1;
        }

        /* CRUMBS Pattern: Using canonical helpers from calculator_ops.h
         * calc_send_*() functions build and send messages using:
         *   crumbs_msg_init() + crumbs_msg_add_u32() + crumbs_controller_send()
         * All take: ctx, addr, write_fn, io_ctx, operation-specific params
         */
        if (strcmp(subcmd, "add") == 0)
            rc = calc_send_add(ctx, addr, crumbs_linux_i2c_write, (void *)lw, a, b);
        else if (strcmp(subcmd, "sub") == 0)
            rc = calc_send_sub(ctx, addr, crumbs_linux_i2c_write, (void *)lw, a, b);
        else if (strcmp(subcmd, "mul") == 0)
            rc = calc_send_mul(ctx, addr, crumbs_linux_i2c_write, (void *)lw, a, b);
        else
            rc = calc_send_div(ctx, addr, crumbs_linux_i2c_write, (void *)lw, a, b);

        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Failed to send (%d)\n", rc);
            return rc;
        }

        printf("OK: %s(%u, %u) sent to 0x%02X. Use 'calculator result' to get answer.\n",
               subcmd, a, b, addr);
        return 0;
    }
    else if (strcmp(subcmd, "result") == 0)
    {
        /* CRUMBS Pattern: SET_REPLY query (2-step)
         * Step 1: calc_query_result() sends opcode 0xFE with query ID 0x80
         *         Peripheral prepares response in its reply buffer
         * Step 2: crumbs_linux_read_message() reads the prepared response
         * This is the standard CRUMBS query pattern for GET operations
         */
        rc = calc_query_result(ctx, addr, crumbs_linux_i2c_write, (void *)lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Query failed (%d)\n", rc);
            return rc;
        }

        /* Platform-specific: crumbs_linux_read_message() wraps I2C read + decode */
        rc = crumbs_linux_read_message(lw, addr, ctx, &reply);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Read failed (%d)\n", rc);
            return rc;
        }

        /* CRUMBS Pattern: Parse response using message helpers
         * crumbs_msg_read_u32() extracts little-endian u32 from payload
         */
        uint32_t result;
        if (crumbs_msg_read_u32(reply.data, reply.data_len, 0, &result) == 0)
            printf("Result: %u\n", result);
        else
            fprintf(stderr, "ERROR: Invalid response\n");

        return 0;
    }
    else if (strcmp(subcmd, "history") == 0)
    {
        rc = calc_query_hist_meta(ctx, addr, crumbs_linux_i2c_write, (void *)lw);
        if (rc != 0)
            return rc;

        rc = crumbs_linux_read_message(lw, addr, ctx, &reply);
        if (rc != 0)
            return rc;

        uint8_t count, write_pos;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &count) != 0 ||
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &write_pos) != 0)
            return -1;

        printf("History: %u entries\n", count);

        for (uint8_t i = 0; i < count; i++)
        {
            rc = calc_query_hist_entry(ctx, addr, crumbs_linux_i2c_write, (void *)lw, i);
            if (rc != 0)
                continue;

            rc = crumbs_linux_read_message(lw, addr, ctx, &reply);
            if (rc != 0 || reply.data_len < 16)
                continue;

            char op[5] = {0};
            memcpy(op, reply.data, 4);
            uint32_t a, b, result;
            crumbs_msg_read_u32(reply.data, reply.data_len, 4, &a);
            crumbs_msg_read_u32(reply.data, reply.data_len, 8, &b);
            crumbs_msg_read_u32(reply.data, reply.data_len, 12, &result);
            printf("  [%u] %s(%u, %u) = %u\n", i, op, a, b, result);
        }
        return 0;
    }

    printf("Unknown calculator command: %s\n", subcmd);
    return -1;
}

/* ============================================================================
 * LED Commands
 * ============================================================================ */

static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    /* Parse device selector */
    uint8_t addr = 0;
    const char *cmd_start = args;

    if (*args == '@')
    {
        unsigned int addr_val;
        if (sscanf(args, "@%i", &addr_val) != 1)
        {
            printf("Usage: led @<addr> <cmd> or led <idx> <cmd>\n");
            return -1;
        }
        addr = (uint8_t)addr_val;
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\n");
            return -1;
        }
        cmd_start++;
    }
    else
    {
        int idx;
        if (sscanf(args, "%d", &idx) != 1)
        {
            printf("Usage: led <idx> <cmd>\n");
            return -1;
        }
        if (find_device(LED_TYPE_ID, idx, &addr) != 0)
        {
            printf("LED #%d not found\n", idx);
            return -1;
        }
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\n");
            return -1;
        }
        cmd_start++;
    }

    while (*cmd_start && isspace((unsigned char)*cmd_start))
        cmd_start++;

    char subcmd[32];
    if (sscanf(cmd_start, "%31s", subcmd) != 1)
    {
        printf("Usage: led <idx|@addr> <set_all|set_one|blink|get_state> [args...]\n");
        return -1;
    }

    const char *rest = cmd_start + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;
    crumbs_message_t reply;

    if (strcmp(subcmd, "set_all") == 0)
    {
        unsigned int mask;
        if (sscanf(rest, "%i", &mask) != 1)
        {
            printf("Usage: led set_all <mask>\n");
            return -1;
        }

        rc = led_send_set_all(ctx, addr, crumbs_linux_i2c_write, (void *)lw, (uint8_t)mask);
        if (rc == 0)
            printf("OK: LEDs at 0x%02X set to 0x%02X\n", addr, (uint8_t)mask);
        return rc;
    }
    else if (strcmp(subcmd, "set_one") == 0)
    {
        unsigned int idx, state;
        if (sscanf(rest, "%u %u", &idx, &state) != 2)
        {
            printf("Usage: led set_one <idx> <state>\n");
            return -1;
        }

        rc = led_send_set_one(ctx, addr, crumbs_linux_i2c_write, (void *)lw, (uint8_t)idx, (uint8_t)state);
        if (rc == 0)
            printf("OK: LED %u at 0x%02X set to %s\n", idx, addr, state ? "ON" : "OFF");
        return rc;
    }
    else if (strcmp(subcmd, "blink") == 0)
    {
        unsigned int idx, enable;
        uint16_t period_ms;
        if (sscanf(rest, "%u %u %hu", &idx, &enable, &period_ms) != 3)
        {
            printf("Usage: led blink <idx> <enable> <period_ms>\n");
            return -1;
        }

        rc = led_send_blink(ctx, addr, crumbs_linux_i2c_write, (void *)lw, (uint8_t)idx, (uint8_t)enable, period_ms);
        if (rc == 0)
            printf("OK: LED %u at 0x%02X blink %s\n", idx, addr, enable ? "enabled" : "disabled");
        return rc;
    }
    else if (strcmp(subcmd, "get_state") == 0)
    {
        rc = led_query_state(ctx, addr, crumbs_linux_i2c_write, (void *)lw);
        if (rc != 0)
            return rc;

        rc = crumbs_linux_read_message(lw, addr, ctx, &reply);
        if (rc != 0)
            return rc;

        uint8_t state;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &state) == 0)
        {
            printf("LED state at 0x%02X: 0x%02X (", addr, state);
            for (int i = 3; i >= 0; i--)
                printf("%d", (state >> i) & 1);
            printf(")\n");
            for (int i = 0; i < 4; i++)
                printf("  LED %d: %s\n", i, (state & (1 << i)) ? "ON" : "OFF");
        }
        return 0;
    }

    printf("Unknown LED command: %s\n", subcmd);
    return -1;
}

/* ============================================================================
 * Servo Commands
 * ============================================================================ */

static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    /* Parse device selector */
    uint8_t addr = 0;
    const char *cmd_start = args;

    if (*args == '@')
    {
        unsigned int addr_val;
        if (sscanf(args, "@%i", &addr_val) != 1)
        {
            printf("Usage: servo @<addr> <cmd> or servo <idx> <cmd>\n");
            return -1;
        }
        addr = (uint8_t)addr_val;
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\n");
            return -1;
        }
        cmd_start++;
    }
    else
    {
        int idx;
        if (sscanf(args, "%d", &idx) != 1)
        {
            printf("Usage: servo <idx> <cmd>\n");
            return -1;
        }
        if (find_device(SERVO_TYPE_ID, idx, &addr) != 0)
        {
            printf("Servo #%d not found\n", idx);
            return -1;
        }
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\n");
            return -1;
        }
        cmd_start++;
    }

    while (*cmd_start && isspace((unsigned char)*cmd_start))
        cmd_start++;

    char subcmd[32];
    if (sscanf(cmd_start, "%31s", subcmd) != 1)
    {
        printf("Usage: servo <idx|@addr> <set_pos|set_speed|sweep|get_pos> [args...]\n");
        return -1;
    }

    const char *rest = cmd_start + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;
    crumbs_message_t reply;

    if (strcmp(subcmd, "set_pos") == 0)
    {
        unsigned int idx, angle;
        if (sscanf(rest, "%u %u", &idx, &angle) != 2)
        {
            printf("Usage: servo set_pos <idx> <angle>\n");
            return -1;
        }

        rc = servo_send_set_pos(ctx, addr, crumbs_linux_i2c_write, (void *)lw, (uint8_t)idx, (uint8_t)angle);
        if (rc == 0)
            printf("OK: Servo %u at 0x%02X position set to %udeg\n", idx, addr, angle);
        return rc;
    }
    else if (strcmp(subcmd, "set_speed") == 0)
    {
        unsigned int idx, speed;
        if (sscanf(rest, "%u %u", &idx, &speed) != 2)
        {
            printf("Usage: servo set_speed <idx> <speed>\n");
            return -1;
        }

        rc = servo_send_set_speed(ctx, addr, crumbs_linux_i2c_write, (void *)lw, (uint8_t)idx, (uint8_t)speed);
        if (rc == 0)
            printf("OK: Servo %u at 0x%02X speed set to %u\n", idx, addr, speed);
        return rc;
    }
    else if (strcmp(subcmd, "sweep") == 0)
    {
        unsigned int idx, enable, min_pos, max_pos, step;
        if (sscanf(rest, "%u %u %u %u %u", &idx, &enable, &min_pos, &max_pos, &step) != 5)
        {
            printf("Usage: servo sweep <idx> <enable> <min> <max> <step>\n");
            return -1;
        }

        rc = servo_send_sweep(ctx, addr, crumbs_linux_i2c_write, (void *)lw,
                              (uint8_t)idx, (uint8_t)enable, (uint8_t)min_pos, (uint8_t)max_pos, (uint8_t)step);
        if (rc == 0)
            printf("OK: Servo %u at 0x%02X sweep %s\n", idx, addr, enable ? "enabled" : "disabled");
        return rc;
    }
    else if (strcmp(subcmd, "get_pos") == 0)
    {
        rc = servo_query_pos(ctx, addr, crumbs_linux_i2c_write, (void *)lw);
        if (rc != 0)
            return rc;

        rc = crumbs_linux_read_message(lw, addr, ctx, &reply);
        if (rc != 0)
            return rc;

        uint8_t pos0, pos1;
        if (reply.data_len >= 2)
        {
            crumbs_msg_read_u8(reply.data, reply.data_len, 0, &pos0);
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &pos1);
            printf("Servo positions at 0x%02X: [0]=%udeg, [1]=%udeg\n", addr, pos0, pos1);
        }
        return 0;
    }

    printf("Unknown servo command: %s\n", subcmd);
    return -1;
}

/* ============================================================================
 * Display Command
 * ============================================================================ */

static int cmd_display(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    uint8_t addr;
    const char *cmd_start;

    /* Parse device selector: @addr or index */
    if (*args == '@')
    {
        unsigned int addr_val;
        if (sscanf(args, "@%i", &addr_val) != 1)
        {
            printf("Usage: display @<addr> <cmd> or display <idx> <cmd>\n");
            return -1;
        }
        addr = (uint8_t)addr_val;
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\n");
            return -1;
        }
        cmd_start++;
    }
    else
    {
        int idx;
        if (sscanf(args, "%d", &idx) != 1)
        {
            printf("Usage: display <idx> <cmd>\n");
            return -1;
        }
        if (find_device(DISPLAY_TYPE_ID, idx, &addr) != 0)
        {
            printf("Display #%d not found\n", idx);
            return -1;
        }
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\n");
            return -1;
        }
        cmd_start++;
    }

    while (*cmd_start && isspace((unsigned char)*cmd_start))
        cmd_start++;

    char subcmd[32];
    if (sscanf(cmd_start, "%31s", subcmd) != 1)
    {
        printf("Usage: display <idx|@addr> <set_number|set_brightness|clear|get_value> [args...]\n");
        return -1;
    }

    const char *rest = cmd_start + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;
    crumbs_message_t msg, reply;

    if (strcmp(subcmd, "set_number") == 0)
    {
        unsigned int number, decimal_pos;
        if (sscanf(rest, "%u %u", &number, &decimal_pos) != 2)
        {
            printf("Usage: display set_number <number> <decimal_pos>\n");
            printf("  number: 0-9999\n");
            printf("  decimal_pos: 0=none, 1=digit1 (left), 2=digit2, 3=digit3, 4=digit4 (right)\n");
            return -1;
        }

        rc = display_build_set_number(&msg, (uint16_t)number, (uint8_t)decimal_pos);
        if (rc != 0)
            return rc;

        rc = crumbs_controller_send(ctx, addr, &msg, crumbs_linux_i2c_write, (void *)lw);
        if (rc == 0)
            printf("OK: Display showing %u (decimal pos %u)\n", number, decimal_pos);
        return rc;
    }
    else if (strcmp(subcmd, "set_brightness") == 0)
    {
        unsigned int level;
        if (sscanf(rest, "%u", &level) != 1)
        {
            printf("Usage: display set_brightness <level>\n");
            printf("  level: 0-10 (0=off, 10=brightest)\n");
            return -1;
        }

        rc = display_build_set_brightness(&msg, (uint8_t)level);
        if (rc != 0)
            return rc;

        rc = crumbs_controller_send(ctx, addr, &msg, crumbs_linux_i2c_write, (void *)lw);
        if (rc == 0)
            printf("OK: Brightness set to %u\n", level);
        return rc;
    }
    else if (strcmp(subcmd, "clear") == 0)
    {
        rc = display_build_clear(&msg);
        if (rc != 0)
            return rc;

        rc = crumbs_controller_send(ctx, addr, &msg, crumbs_linux_i2c_write, (void *)lw);
        if (rc == 0)
            printf("OK: Display cleared\n");
        return rc;
    }
    else if (strcmp(subcmd, "get_value") == 0)
    {
        rc = display_build_get_value(&msg);
        if (rc != 0)
            return rc;

        rc = crumbs_controller_send(ctx, addr, &msg, crumbs_linux_i2c_write, (void *)lw);
        if (rc != 0)
            return rc;

        rc = crumbs_linux_read_message(lw, addr, ctx, &reply);
        if (rc != 0)
            return rc;

        uint16_t number;
        uint8_t decimal_pos, brightness;
        rc = display_parse_get_value(reply.data, reply.data_len, &number, &decimal_pos, &brightness);
        if (rc == 0)
        {
            printf("Display: number=%u, decimal=%u, brightness=%u\n", number, decimal_pos, brightness);
        }
        return rc;
    }

    printf("Unknown display command: %s\n", subcmd);
    return -1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[])
{
    const char *i2c_device = (argc > 1) ? argv[1] : "/dev/i2c-1";

    /* CRUMBS Pattern: Initialize controller
     * crumbs_linux_init_controller() - Initialize platform HAL (Linux/i2c-dev)
     * This internally calls crumbs_init() with CONTROLLER role
     * Returns platform handle used for all I2C operations
     */
    crumbs_context_t ctx;
    crumbs_linux_i2c_t lw;
    int rc = crumbs_linux_init_controller(&ctx, &lw, i2c_device, 100000);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR: Failed to open I2C device '%s' (%d)\n", i2c_device, rc);
        fprintf(stderr, "       Try: sudo chmod 666 %s\n", i2c_device);
        return 1;
    }

    /* Load device configuration */
    g_device_count = 0;
    int type_counts[256] = {0};
    for (size_t i = 0; i < DEVICE_CONFIG_COUNT && g_device_count < 16; i++)
    {
        device_info_t *dev = &g_devices[g_device_count];
        dev->type_id = DEVICE_CONFIG[i].type_id;
        dev->addr = DEVICE_CONFIG[i].addr;
        dev->index = type_counts[dev->type_id]++;

        if (dev->type_id == CALC_TYPE_ID)
            dev->name = "Calculator";
        else if (dev->type_id == LED_TYPE_ID)
            dev->name = "LED";
        else if (dev->type_id == SERVO_TYPE_ID)
            dev->name = "Servo";
        else if (dev->type_id == DISPLAY_TYPE_ID)
            dev->name = "Display";
        else
            dev->name = "Unknown";

        g_device_count++;
    }

    printf("\nLHWIT Manual Controller\n");
    printf("=======================\n");
    printf("I2C Device: %s\n", i2c_device);
    printf("Loaded %d device(s) from config.h\n", g_device_count);
    printf("\nType 'list' to see devices, 'help' for commands.\n\n");

    char line[MAX_CMD_LEN];
    while (1)
    {
        printf("lhwit> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        trim_whitespace(line);
        if (strlen(line) == 0)
            continue;

        char cmd[64];
        if (sscanf(line, "%63s", cmd) != 1)
            continue;

        const char *args = line + strlen(cmd);
        while (*args && isspace((unsigned char)*args))
            args++;

        if (strcmp(cmd, "help") == 0)
            print_help();
        else if (strcmp(cmd, "list") == 0)
            cmd_list();
        else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0)
        {
            printf("Goodbye!\n");
            break;
        }
        else if (strcmp(cmd, "calculator") == 0)
            cmd_calculator(&ctx, &lw, args);
        else if (strcmp(cmd, "led") == 0)
            cmd_led(&ctx, &lw, args);
        else if (strcmp(cmd, "servo") == 0)
            cmd_servo(&ctx, &lw, args);
        else if (strcmp(cmd, "display") == 0)
            cmd_display(&ctx, &lw, args);
        else
            printf("Unknown command: %s (type 'help')\n", cmd);
    }

    crumbs_linux_close(&lw);
    return 0;
}
