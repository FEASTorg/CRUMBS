/**
 * @file main.c
 * @brief Manual (preconfigured) controller for lhwit_family peripherals.
 *
 * This is a complete interactive application demonstrating standard CRUMBS usage:
 * - Preconfigured addresses from config.h (typical production pattern)
 * - Command sending via canonical *_ops.h helper functions
 * - SET_REPLY query pattern (write query opcode, then read response)
 * - Platform-specific reading via crumbs_linux_read_message()
 *
 * Structure:
 * - Configuration: Fixed addresses in config.h
 * - Commands: Use canonical helper functions from *_ops.h
 * - Queries: Use SET_REPLY pattern (2-step: query write + response read)
 * - UI: Interactive shell (application layer, not library code)
 *
 * Difference from controller_discovery:
 * - No scan command (addresses are preconfigured)
 * - No device-found checks (assumes devices exist at configured addresses)
 * - Faster startup (no bus scan required)
 *
 * Usage:
 *   ./controller_manual [/dev/i2c-1]
 *   > calculator add 10 20
 *   > led set_all 0x0F
 *   > servo set_pos 0 90
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

/* Preconfigured device addresses from config.h */
static uint8_t g_calc_addr = CALCULATOR_ADDR;
static uint8_t g_led_addr = LED_ADDR;
static uint8_t g_servo_addr = SERVO_ADDR;

#define MAX_CMD_LEN 256

/* ============================================================================
 * Forward Declarations
 * ============================================================================ */

static void print_help(void);
static void trim_whitespace(char *str);
static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);
static int cmd_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args);

/* ============================================================================
 * Help
 * ============================================================================ */

static void print_help(void)
{
    printf("\nLHWIT Manual Controller Commands\n");
    printf("=================================\n\n");
    printf("General:\n");
    printf("  help                              - Show this help\n");
    printf("  quit, exit                        - Exit\n\n");
    
    printf("Calculator (0x%02X):\n", g_calc_addr);
    printf("  calculator add <a> <b>            - Add\n");
    printf("  calculator sub <a> <b>            - Subtract\n");
    printf("  calculator mul <a> <b>            - Multiply\n");
    printf("  calculator div <a> <b>            - Divide\n");
    printf("  calculator result                 - Get last result\n");
    printf("  calculator history                - Show history\n\n");
    
    printf("LED (0x%02X):\n", g_led_addr);
    printf("  led set_all <mask>                - Set all LEDs (0x0F = all on)\n");
    printf("  led set_one <idx> <0|1>           - Set single LED\n");
    printf("  led blink <idx> <en> <ms>         - Configure blink\n");
    printf("  led get_state                     - Get LED state\n\n");
    
    printf("Servo (0x%02X):\n", g_servo_addr);
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
 * Calculator Commands
 * ============================================================================ */

static int cmd_calculator(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw, const char *args)
{
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
    
    printf("\nLHWIT Manual Controller\n");
    printf("=======================\n");
    printf("I2C Device: %s\n", i2c_device);
    printf("Configured addresses:\n");
    printf("  Calculator: 0x%02X\n", g_calc_addr);
    printf("  LED:        0x%02X\n", g_led_addr);
    printf("  Servo:      0x%02X\n", g_servo_addr);
    printf("\nType 'help' for commands.\n\n");
    
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
