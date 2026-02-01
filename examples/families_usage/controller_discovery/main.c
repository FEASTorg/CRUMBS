/**
 * @file main.c
 * @brief Discovery-based controller for lhwit_family peripherals.
 *
 * This is a complete interactive application demonstrating proper CRUMBS usage:
 * - Auto-discovery via crumbs_controller_scan_for_crumbs_with_types()
 * - Command sending via canonical *_ops.h helper functions
 * - SET_REPLY query pattern (write query opcode, then read response)
 * - Platform-specific reading via crumbs_linux_read_message()
 *
 * Structure:
 * - Scan: Uses core scanner to find devices by type ID
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

/* Discovered device addresses (0 = not found) */
static uint8_t g_calc_addr = 0;
static uint8_t g_led_addr = 0;
static uint8_t g_servo_addr = 0;

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
static void trim_whitespace(char *str);
static int cmd_scan(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw);
static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);

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
    printf("  quit, exit                        - Exit\n\n");
    
    if (g_calc_addr)
        printf("Calculator (0x%02X):\n", g_calc_addr);
    else
        printf("Calculator (not found - run scan):\n");
    printf("  calculator add <a> <b>            - Add\n");
    printf("  calculator sub <a> <b>            - Subtract\n");
    printf("  calculator mul <a> <b>            - Multiply\n");
    printf("  calculator div <a> <b>            - Divide\n");
    printf("  calculator result                 - Get last result\n");
    printf("  calculator history                - Show history\n\n");
    
    if (g_led_addr)
        printf("LED (0x%02X):\n", g_led_addr);
    else
        printf("LED (not found - run scan):\n");
    printf("  led set_all <mask>                - Set all LEDs (0x0F = all on)\n");
    printf("  led set_one <idx> <0|1>           - Set single LED\n");
    printf("  led blink <idx> <en> <ms>         - Configure blink\n");
    printf("  led get_state                     - Get LED state\n\n");
    
    if (g_servo_addr)
        printf("Servo (0x%02X):\n", g_servo_addr);
    else
        printf("Servo (not found - run scan):\n");
    printf("  servo set_pos <idx> <angle>       - Set position (0-180°)\n");
    printf("  servo set_speed <idx> <speed>     - Set speed (0-20)\n");
    printf("  servo sweep <i> <en> <min> <max> <step> - Configure sweep\n");
    printf("  servo get_pos                     - Get positions\n\n");
}

static void trim_whitespace(char *str)
{
    if (!str) return;
    char *start = str;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == '\0') { str[0] = '\0'; return; }
    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    size_t len = (size_t)(end - start + 1);
    memmove(str, start, len);
    str[len] = '\0';
}

/* ============================================================================
 * Scan Command
 * ============================================================================ */

static int cmd_scan(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw)
{
    uint8_t addrs[16];
    uint8_t types[16];

    printf("Scanning I2C bus for CRUMBS devices...\n");

    /* CRUMBS Pattern: Type-aware scanning
     * crumbs_controller_scan_for_crumbs_with_types() probes addresses and
     * queries type IDs via message header. Returns both addresses and types.
     * - Addresses 0x08-0x77 (valid I2C range)
     * - strict=0: Accept any CRUMBS device
     * - Platform-specific write/read functions
     * - 100ms timeout per device
     */
    int count = crumbs_controller_scan_for_crumbs_with_types(
        ctx, 0x08, 0x77, 0,
        crumbs_linux_i2c_write, crumbs_linux_read, (void *)lw,
        addrs, types, 16, 100000);

    if (count < 0)
    {
        fprintf(stderr, "ERROR: Scan failed (%d)\n", count);
        g_calc_addr = g_led_addr = g_servo_addr = 0;
        g_device_count = 0;
        return count;
    }

    if (count == 0)
    {
        printf("No devices found.\n");
        g_calc_addr = g_led_addr = g_servo_addr = 0;
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

    /* Store addresses only for compatible devices */
    g_calc_addr = g_led_addr = g_servo_addr = 0;
    for (int i = 0; i < g_device_count; i++)
    {
        if (g_devices[i].compat == 0)
        {
            if (g_devices[i].type_id == CALC_TYPE_ID)
                g_calc_addr = g_devices[i].addr;
            if (g_devices[i].type_id == LED_TYPE_ID)
                g_led_addr = g_devices[i].addr;
            if (g_devices[i].type_id == SERVO_TYPE_ID)
                g_servo_addr = g_devices[i].addr;
        }
    }

    return usable;
}

/* ============================================================================
 * Device Compatibility Check
 * ============================================================================ */

/**
 * @brief Check if device is compatible before sending commands.
 * @return 0 if OK, -1 if not found or incompatible (prints error message).
 */
static int check_device_compat(uint8_t addr, const char *name)
{
    if (addr == 0)
    {
        printf("%s not found. Run 'scan' first.\n", name);
        return -1;
    }
    for (int i = 0; i < g_device_count; i++)
    {
        if (g_devices[i].addr == addr && g_devices[i].compat < 0)
        {
            printf("%s at 0x%02X is incompatible.\n", name, addr);
            printf("Update firmware/headers and run 'scan' again.\n");
            return -1;
        }
    }
    return 0;
}

/* ============================================================================
 * Calculator Commands
 * ============================================================================ */

static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
    if (check_device_compat(g_calc_addr, "Calculator") != 0)
        return -1;
    
    char subcmd[32];
    if (sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: calculator <add|sub|mul|div|result|history> [args...]\n");
        return -1;
    }
    
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
    int rc;
    crumbs_message_t reply;
    
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
            rc = calc_send_add(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);
        else if (strcmp(subcmd, "sub") == 0)
            rc = calc_send_sub(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);
        else if (strcmp(subcmd, "mul") == 0)
            rc = calc_send_mul(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);
        else
            rc = calc_send_div(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, a, b);
        
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
        /* CRUMBS Pattern: SET_REPLY query (2-step)
         * Step 1: calc_query_result() sends opcode 0xFE with query ID 0x80
         *         Peripheral prepares response in its reply buffer
         * Step 2: crumbs_linux_read_message() reads the prepared response
         * This is the standard CRUMBS query pattern for GET operations
         */
        rc = calc_query_result(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0)
        {
            fprintf(stderr, "ERROR: Query failed (%d)\n", rc);
            return rc;
        }
        
        /* Platform-specific: crumbs_linux_read_message() wraps I2C read + decode */
        rc = crumbs_linux_read_message(lw, g_calc_addr, ctx, &reply);
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
        rc = calc_query_hist_meta(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0) return rc;
        
        rc = crumbs_linux_read_message(lw, g_calc_addr, ctx, &reply);
        if (rc != 0) return rc;
        
        uint8_t count, write_pos;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &count) != 0 ||
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &write_pos) != 0)
            return -1;
        
        printf("History: %u entries\n", count);
        
        for (uint8_t i = 0; i < count; i++)
        {
            rc = calc_query_hist_entry(ctx, g_calc_addr, crumbs_linux_i2c_write, (void*)lw, i);
            if (rc != 0) continue;
            
            rc = crumbs_linux_read_message(lw, g_calc_addr, ctx, &reply);
            if (rc != 0 || reply.data_len < 16) continue;
            
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
    if (check_device_compat(g_led_addr, "LED") != 0)
        return -1;
    
    char subcmd[32];
    if (sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: led <set_all|set_one|blink|get_state> [args...]\n");
        return -1;
    }
    
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
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
        
        rc = led_send_set_all(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw, (uint8_t)mask);
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
        
        rc = led_send_set_one(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw, (uint8_t)idx, (uint8_t)state);
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
        
        rc = led_send_blink(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw, (uint8_t)idx, (uint8_t)enable, period_ms);
        if (rc == 0)
            printf("OK: LED %u blink %s\n", idx, enable ? "enabled" : "disabled");
        return rc;
    }
    else if (strcmp(subcmd, "get_state") == 0)
    {
        rc = led_query_state(ctx, g_led_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0) return rc;
        
        rc = crumbs_linux_read_message(lw, g_led_addr, ctx, &reply);
        if (rc != 0) return rc;
        
        uint8_t state;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &state) == 0)
        {
            printf("LED state: 0x%02X (", state);
            for (int i = 3; i >= 0; i--) printf("%d", (state >> i) & 1);
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
    if (check_device_compat(g_servo_addr, "Servo") != 0)
        return -1;
    
    char subcmd[32];
    if (sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Usage: servo <set_pos|set_speed|sweep|get_pos> [args...]\n");
        return -1;
    }
    
    const char *rest = args + strlen(subcmd);
    while (*rest && isspace((unsigned char)*rest)) rest++;
    
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
        
        rc = servo_send_set_pos(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw, (uint8_t)idx, (uint8_t)angle);
        if (rc == 0)
            printf("OK: Servo %u position set to %u°\n", idx, angle);
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
        
        rc = servo_send_set_speed(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw, (uint8_t)idx, (uint8_t)speed);
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
        
        rc = servo_send_sweep(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw,
                              (uint8_t)idx, (uint8_t)enable, (uint8_t)min_pos, (uint8_t)max_pos, (uint8_t)step);
        if (rc == 0)
            printf("OK: Servo %u sweep %s\n", idx, enable ? "enabled" : "disabled");
        return rc;
    }
    else if (strcmp(subcmd, "get_pos") == 0)
    {
        rc = servo_query_pos(ctx, g_servo_addr, crumbs_linux_i2c_write, (void*)lw);
        if (rc != 0) return rc;
        
        rc = crumbs_linux_read_message(lw, g_servo_addr, ctx, &reply);
        if (rc != 0) return rc;
        
        uint8_t pos0, pos1;
        if (reply.data_len >= 2)
        {
            crumbs_msg_read_u8(reply.data, reply.data_len, 0, &pos0);
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &pos1);
            printf("Servo positions: [0]=%u°, [1]=%u°\n", pos0, pos1);
        }
        return 0;
    }
    
    printf("Unknown servo command: %s\n", subcmd);
    return -1;
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[])
{
    const char *i2c_device = (argc > 1) ? argv[1] : "/dev/i2c-1";
    
    /* CRUMBS Pattern: Initialize controller
     * 1. crumbs_init() - Initialize core context (role=CONTROLLER, no I2C addr)
     * 2. crumbs_linux_init_controller() - Initialize platform HAL (Linux/i2c-dev)
     *    Returns platform handle used for all I2C operations
     */
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);
    
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
        
        if (!fgets(line, sizeof(line), stdin)) break;
        
        trim_whitespace(line);
        if (strlen(line) == 0) continue;
        
        char cmd[64];
        if (sscanf(line, "%63s", cmd) != 1) continue;
        
        const char *args = line + strlen(cmd);
        while (*args && isspace((unsigned char)*args)) args++;
        
        if (strcmp(cmd, "help") == 0)
            print_help();
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
        else
            printf("Unknown command: %s (type 'help')\n", cmd);
    }
    
    crumbs_linux_close(&lw);
    return 0;
}
