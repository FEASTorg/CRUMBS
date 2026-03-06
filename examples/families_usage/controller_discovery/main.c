/**
 * @file main.c
 * @brief Discovery-based controller for lhwit_family peripherals.
 *
 * This is a complete interactive application demonstrating proper CRUMBS usage:
 * - Auto-discovery via crumbs_linux_scan_for_crumbs_with_types()
 * - Command sending via canonical *_ops.h helper functions
 * - SET_REPLY query pattern via *_get_*() wrapper functions
 * - Platform-specific delay via crumbs_linux_delay_us()
 *
 * Structure:
 * - Scan: Uses Linux HAL scanner (suppresses noise from empty addresses)
 * - Commands: Use canonical helper functions from *_ops.h
 * - Queries: Use SET_REPLY pattern (2-step: query write + response read)
 * - UI: Interactive shell (application layer, not library code)
 *
 * Usage:
 *   ./controller_discovery [/dev/i2c-1]
 *   > scan
 *   > calculator add 10 20
 *   > led set_all 0x0F
 *   > servo set_pos 0 90
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "crumbs.h"
#include "crumbs_linux.h"
#include "crumbs_message_helpers.h"

#include "../lhwit_family/lhwit_ops.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

/* Device info with version compatibility status */
typedef struct
{
    uint8_t addr;
    uint8_t type_id;
    uint16_t crumbs_ver;
    uint8_t mod_major;
    uint8_t mod_minor;
    uint8_t mod_patch;
    int compat; /* 0=OK, <0=error */
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
static int cmd_scan(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw);
static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_display(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);

/* ============================================================================
 * Help
 * ============================================================================ */

static void print_help(void)
{
    printf("\nLHWIT Discovery Controller Commands\n");
    printf("====================================\n\n");
    printf("General:\n");
    printf("  help                              - Show this help\n");
    printf("  scan                              - Scan I2C bus for devices\n");
    printf("  list                              - List discovered devices\n");
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
    printf("  led 0 get_state                   - Get LED state\n");
    printf("  led 0 get_blink                   - Get blink config\n\n");

    printf("Servo:\n");
    printf("  servo 0 set_pos <idx> <angle>     - Set position (0-180deg)\n");
    printf("  servo 0 set_speed <idx> <speed>   - Set speed (0-20)\n");
    printf("  servo 0 sweep <i> <en> <min> <max> <step> - Configure sweep\n");
    printf("  servo 0 get_pos                   - Get positions\n");
    printf("  servo 0 get_speed                 - Get speeds\n\n");

    printf("Display:\n");
    printf("  display 0 set_number <num> <dec>  - Display number (dec=0-4)\n");
    printf("  display 0 set_brightness <level>  - Set brightness (0-10)\n");
    printf("  display 0 clear                   - Clear display\n");
    printf("  display 0 get_value               - Get current value\n\n");
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
 * List Command
 * ============================================================================ */

static void cmd_list(void)
{
    if (g_device_count == 0)
    {
        printf("No devices found. Run 'scan' first.\n");
        return;
    }

    printf("\nDiscovered Devices:\n");
    printf("-------------------\n");

    int type_counts[256] = {0};
    for (int i = 0; i < g_device_count; i++)
    {
        const char *name = "Unknown";
        if (g_devices[i].type_id == CALC_TYPE_ID)
            name = "Calculator";
        else if (g_devices[i].type_id == LED_TYPE_ID)
            name = "LED";
        else if (g_devices[i].type_id == SERVO_TYPE_ID)
            name = "Servo";
        else if (g_devices[i].type_id == DISPLAY_TYPE_ID)
            name = "Display";

        int idx = type_counts[g_devices[i].type_id]++;
        const char *compat_str = g_devices[i].compat == 0 ? "OK" : "INCOMPATIBLE";

        printf("[%d] %s #%d at 0x%02X (Type 0x%02X) - %s\n",
               i, name, idx, g_devices[i].addr, g_devices[i].type_id, compat_str);
    }
    printf("\n");
}

/* ============================================================================
 * Device Lookup
 * ============================================================================ */

/**
 * @brief Find device address by type and index among devices of that type.
 * @param type_id  Device type ID
 * @param index    Index among compatible devices of same type (0-based)
 * @param addr_out Output address
 * @return 0 on success, -1 if not found
 */
static int find_device(uint8_t type_id, int index, uint8_t *addr_out)
{
    int type_count = 0;
    for (int i = 0; i < g_device_count; i++)
    {
        if (g_devices[i].type_id == type_id && g_devices[i].compat == 0)
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

/* ============================================================================
 * Scan Command
 * ============================================================================ */

static int cmd_scan(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw)
{
    uint8_t addrs[16];
    uint8_t types[16];

    printf("Scanning I2C bus for CRUMBS devices (0x08-0x77)...\n");

    /* CRUMBS Pattern: Linux scan wrapper
     * crumbs_linux_scan_for_crumbs_with_types() wraps the core scanner and
     * automatically suppresses expected I/O errors from empty addresses.
     * - Addresses 0x08-0x77 (valid I2C range)
     * - strict=0: Accept any CRUMBS device
     * - 100ms timeout per device
     */
    int count = crumbs_linux_scan_for_crumbs_with_types(
        ctx, lw, 0x08, 0x77, 0,
        addrs, types, 16, 100000);

    if (count < 0)
    {
        fprintf(stderr, "ERROR: Scan failed (%d)\n", count);
        g_device_count = 0;
        return count;
    }

    if (count == 0)
    {
        printf("No devices found.\n");
        g_device_count = 0;
        return 0;
    }

    printf("\nFound %d device(s):\n", count);
    printf("--------------------------------------------\n");

    g_device_count = count;
    int usable = 0;

    for (int i = 0; i < count; i++)
    {
        device_info_t *dev = &g_devices[i];
        dev->addr = addrs[i];
        dev->type_id = types[i];
        dev->compat = -99; /* Unknown until we query */

        /* Get type name and expected version */
        const char *name = "Unknown";
        uint8_t exp_major = 0, exp_minor = 0;

        if (types[i] == CALC_TYPE_ID)
        {
            name = "Calculator";
            exp_major = CALC_MODULE_VER_MAJOR;
            exp_minor = CALC_MODULE_VER_MINOR;
        }
        else if (types[i] == LED_TYPE_ID)
        {
            name = "LED";
            exp_major = LED_MODULE_VER_MAJOR;
            exp_minor = LED_MODULE_VER_MINOR;
        }
        else if (types[i] == SERVO_TYPE_ID)
        {
            name = "Servo";
            exp_major = SERVO_MODULE_VER_MAJOR;
            exp_minor = SERVO_MODULE_VER_MINOR;
        }
        else if (types[i] == DISPLAY_TYPE_ID)
        {
            name = "Display";
            exp_major = DISPLAY_MODULE_VER_MAJOR;
            exp_minor = DISPLAY_MODULE_VER_MINOR;
        }

        printf("[0x%02X] %s\n", addrs[i], name);

        /* Query version (opcode 0x00) via SET_REPLY */
        crumbs_message_t query, reply;
        crumbs_msg_init(&query, 0, CRUMBS_CMD_SET_REPLY);
        crumbs_msg_add_u8(&query, 0x00);

        uint8_t buf[8];
        size_t len = crumbs_encode_message(&query, buf, sizeof(buf));
        crumbs_linux_i2c_write(lw, addrs[i], buf, len);
        usleep(10000); /* Give peripheral time to prepare reply */

        int rc = crumbs_linux_read_message(lw, addrs[i], ctx, &reply);
        if (rc != 0)
        {
            printf("       ! Version query failed\n");
            dev->compat = -99;
            continue;
        }

        /* Parse version */
        if (lhwit_parse_version(reply.data, reply.data_len,
                                &dev->crumbs_ver, &dev->mod_major,
                                &dev->mod_minor, &dev->mod_patch) != 0)
        {
            printf("       ! Invalid version format\n");
            dev->compat = -99;
            continue;
        }

        /* Display version info */
        char ver_str[12], ctrl_ver_str[12];
        lhwit_format_version(dev->crumbs_ver, ver_str);
        lhwit_format_version(CRUMBS_VERSION, ctrl_ver_str);

        printf("       CRUMBS: v%s (controller: v%s)\n", ver_str, ctrl_ver_str);
        printf("       Module: v%d.%d.%d (expected: v%d.%d.x)\n",
               dev->mod_major, dev->mod_minor, dev->mod_patch,
               exp_major, exp_minor);

        /* Check compatibility */
        int crumbs_ok = lhwit_check_crumbs_compat(dev->crumbs_ver);
        int module_ok = lhwit_check_module_compat(dev->mod_major, dev->mod_minor,
                                                  exp_major, exp_minor);

        if (crumbs_ok < 0)
        {
            printf("       X CRUMBS version too old\n");
            printf("         -> Update peripheral firmware\n");
            dev->compat = -1;
        }
        else if (module_ok == -1)
        {
            printf("       X Module major version mismatch\n");
            if (dev->mod_major > exp_major)
                printf("         -> Recompile controller with new header\n");
            else
                printf("         -> Update peripheral firmware\n");
            dev->compat = -2;
        }
        else if (module_ok == -2)
        {
            printf("       X Module minor version too old\n");
            printf("         -> Update peripheral firmware\n");
            dev->compat = -3;
        }
        else
        {
            printf("       OK Compatible\n");
            dev->compat = 0;
            usable++;
        }
    }

    printf("--------------------------------------------\n");
    printf("Usable: %d/%d devices\n\n", usable, count);

    return usable;
}

/* ============================================================================
 * Device Compatibility Check
 * ============================================================================ */

/**
 * @brief Check if device is compatible before sending commands.
 * @return 0 if OK, -1 if incompatible (prints error message).
 */
static int check_device_compat(uint8_t addr)
{
    for (int i = 0; i < g_device_count; i++)
    {
        if (g_devices[i].addr == addr)
        {
            if (g_devices[i].compat < 0)
            {
                printf("Device at 0x%02X is incompatible.\n", addr);
                printf("Update firmware/headers and run 'scan' again.\n");
                return -1;
            }
            return 0;
        }
    }
    printf("Device at 0x%02X not found in scan results.\n", addr);
    return -1;
}

/* ============================================================================
 * Calculator Commands
 * ============================================================================ */

static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    /* Parse device selector */
    uint8_t addr = 0;
    const char *cmd_start = args;

    if (*args == '@')
    {
        unsigned int addr_val;
        if (sscanf(args, "@%i", &addr_val) != 1)
        {
            printf("Usage: calculator @<addr> <cmd> or calculator <idx> <cmd>\n");
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
            printf("Usage: calculator <idx> <cmd>\n");
            return -1;
        }
        if (find_device(CALC_TYPE_ID, idx, &addr) != 0)
        {
            printf("Calculator #%d not found\n", idx);
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

    if (check_device_compat(addr) != 0)
        return -1;

    while (*cmd_start && isspace((unsigned char)*cmd_start))
        cmd_start++;

    char subcmd[32];
    if (sscanf(cmd_start, "%31s", subcmd) != 1)
    {
        printf("Usage: calculator <idx|@addr> <add|sub|mul|div|result|history> [args...]\n");
        return -1;
    }

    const char *rest = cmd_start + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;

    if (strcmp(subcmd, "add") == 0 || strcmp(subcmd, "sub") == 0 ||
        strcmp(subcmd, "mul") == 0 || strcmp(subcmd, "div") == 0)
    {
        uint32_t a, b;
        if (sscanf(rest, "%u %u", &a, &b) != 2)
        {
            printf("Usage: calculator %s <a> <b>\n", subcmd);
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

        printf("OK: %s(%u, %u) sent. Use 'calculator result' to get answer.\n", subcmd, a, b);
        return 0;
    }
    else if (strcmp(subcmd, "result") == 0)
    {
        calc_result_t res;
        rc = calc_get_result(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                             crumbs_linux_delay_us, (void *)lw, &res);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: calc_get_result failed (%d)\n", rc);
            return rc;
        }
        printf("Result: %u\n", res.result);
        return 0;
    }
    else if (strcmp(subcmd, "history") == 0)
    {
        calc_hist_meta_t meta;
        rc = calc_get_hist_meta(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                                crumbs_linux_delay_us, (void *)lw, &meta);
        if (rc != 0)
            return rc;

        printf("History: %u entries\n", meta.count);

        for (uint8_t i = 0; i < meta.count; i++)
        {
            calc_hist_entry_t entry;
            rc = calc_get_hist_entry(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                                     crumbs_linux_delay_us, (void *)lw, i, &entry);
            if (rc != 0)
                continue;
            printf("  [%u] %s(%u, %u) = %u\n", i, entry.op, entry.a, entry.b, entry.result);
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
            printf("Usage: led @<addr> <cmd> or led <idx> <cmd>\\n");
            return -1;
        }
        addr = (uint8_t)addr_val;
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\\n");
            return -1;
        }
        cmd_start++;
    }
    else
    {
        int idx;
        if (sscanf(args, "%d", &idx) != 1)
        {
            printf("Usage: led <idx> <cmd>\\n");
            return -1;
        }
        if (find_device(LED_TYPE_ID, idx, &addr) != 0)
        {
            printf("LED #%d not found\\n", idx);
            return -1;
        }
        cmd_start = strchr(args, ' ');
        if (!cmd_start)
        {
            printf("Missing command\\n");
            return -1;
        }
        cmd_start++;
    }

    if (check_device_compat(addr) != 0)
        return -1;

    while (*cmd_start && isspace((unsigned char)*cmd_start))
        cmd_start++;

    char subcmd[32];
    if (sscanf(cmd_start, "%31s", subcmd) != 1)
    {
        printf("Usage: led <idx|@addr> <set_all|set_one|blink|get_state|get_blink> [args...]\\n");
        return -1;
    }

    const char *rest = cmd_start + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;

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
            printf("OK: LEDs set to 0x%02X\n", (uint8_t)mask);
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
            printf("OK: LED %u set to %s\n", idx, state ? "ON" : "OFF");
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
            printf("OK: LED %u blink %s\n", idx, enable ? "enabled" : "disabled");
        return rc;
    }
    else if (strcmp(subcmd, "get_state") == 0)
    {
        led_state_result_t res;
        rc = led_get_state(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                           crumbs_linux_delay_us, (void *)lw, &res);
        if (rc != 0)
            return rc;

        printf("LED state: 0x%02X (", res.states);
        for (int i = 3; i >= 0; i--)
            printf("%d", (res.states >> i) & 1);
        printf(")\n");
        for (int i = 0; i < 4; i++)
            printf("  LED %d: %s\n", i, (res.states & (1 << i)) ? "ON" : "OFF");
        return 0;
    }
    else if (strcmp(subcmd, "get_blink") == 0)
    {
        led_blink_result_t res;
        rc = led_get_blink(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                           crumbs_linux_delay_us, (void *)lw, &res);
        if (rc != 0)
            return rc;

        for (int i = 0; i < 4; i++)
            printf("  LED %d: blink=%s, period=%ums\n", i,
                   res.enable[i] ? "ON" : "OFF", res.period_ms[i]);
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

    if (check_device_compat(addr) != 0)
        return -1;

    while (*cmd_start && isspace((unsigned char)*cmd_start))
        cmd_start++;

    char subcmd[32];
    if (sscanf(cmd_start, "%31s", subcmd) != 1)
    {
        printf("Usage: servo <idx|@addr> <set_pos|set_speed|sweep|get_pos|get_speed> [args...]\n");
        return -1;
    }

    const char *rest = cmd_start + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest))
        rest++;

    int rc;

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
            printf("OK: Servo %u position set to %udeg\n", idx, angle);
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
            printf("OK: Servo %u speed set to %u\n", idx, speed);
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
            printf("OK: Servo %u sweep %s\n", idx, enable ? "enabled" : "disabled");
        return rc;
    }
    else if (strcmp(subcmd, "get_pos") == 0)
    {
        servo_pos_result_t res;
        rc = servo_get_pos(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                           crumbs_linux_delay_us, (void *)lw, &res);
        if (rc != 0)
            return rc;

        printf("Servo positions: [0]=%udeg, [1]=%udeg\n", res.pos[0], res.pos[1]);
        return 0;
    }
    else if (strcmp(subcmd, "get_speed") == 0)
    {
        servo_speed_result_t res;
        rc = servo_get_speed(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                             crumbs_linux_delay_us, (void *)lw, &res);
        if (rc != 0)
            return rc;

        printf("Servo speeds: [0]=%u, [1]=%u\n", res.speed[0], res.speed[1]);
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

    if (check_device_compat(addr) != 0)
        return -1;

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
    crumbs_message_t msg;

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
        display_value_result_t res;
        rc = display_get_value(ctx, addr, crumbs_linux_i2c_write, crumbs_linux_read,
                               crumbs_linux_delay_us, (void *)lw, &res);
        if (rc != 0)
            return rc;

        printf("Display: number=%u, decimal=%u, brightness=%u\n",
               res.number, res.decimal_pos, res.brightness);
        return 0;
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

    printf("\nLHWIT Discovery Controller\n");
    printf("==========================\n");
    printf("I2C Device: %s\n", i2c_device);
    printf("Type 'scan' to discover devices, 'help' for commands.\n\n");

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
        else if (strcmp(cmd, "scan") == 0)
            cmd_scan(&ctx, &lw);
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
