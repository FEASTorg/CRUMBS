/**
 * @file main.c
 * @brief LHWIT Manual Controller - Hardcoded Address Configuration
 *
 * This controller uses hardcoded addresses from config.h instead of
 * performing bus scans. This provides faster startup and simpler code
 * but requires knowing the device addresses in advance.
 *
 * Supports:
 *   - Calculator (Type 0x03): 32-bit integer math with history
 *   - LED Array (Type 0x01): 4-LED control
 *   - Servo Controller (Type 0x02): 2-servo position control
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "crumbs.h"
#include "crumbs_linux.h"
#include "lhwit_ops.h"
#include "config.h"

/* ============================================================================
 * Global State
 * ============================================================================ */

static crumbs_context_t g_ctx;
static crumbs_linux_i2c_t g_lw;

static uint8_t g_calc_addr = CALCULATOR_ADDR;
static uint8_t g_led_addr = LED_ADDR;
static uint8_t g_servo_addr = SERVO_ADDR;

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * @brief Trim leading and trailing whitespace from a string (in-place)
 */
static void trim_whitespace(char *str)
{
    if (!str)
        return;

    char *start = str;
    while (*start && isspace((unsigned char)*start))
        start++;

    char *end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end))
        end--;

    size_t len = end - start + 1;
    memmove(str, start, len);
    str[len] = '\0';
}

/**
 * @brief Display LED state in binary format
 */
static void print_led_state_binary(uint8_t state)
{
    printf("LED state: 0x%02X (", state);
    for (int i = 3; i >= 0; i--)
        printf("%d", (state >> i) & 1);
    printf(")\n");

    for (int i = 0; i < 4; i++)
    {
        printf("  LED %d: %s", i, (state & (1 << i)) ? "ON" : "OFF");
        if (i < 3)
            printf(",");
    }
    printf("\n");
}

static void print_help(void)
{
    printf("\n");
    printf("LHWIT Manual Controller - Command Reference\n");
    printf("===========================================\n");
    printf("\n");
    printf("Configured Addresses:\n");
    printf("  Calculator: 0x%02X  |  LED: 0x%02X  |  Servo: 0x%02X\n",
           g_calc_addr, g_led_addr, g_servo_addr);
    printf("\n");
    printf("General:\n");
    printf("  help                              - Show this help\n");
    printf("  quit, exit                        - Exit the controller\n");
    printf("\n");
    printf("Calculator (0x%02X):\n", g_calc_addr);
    printf("  calculator add <a> <b>            - Add two numbers\n");
    printf("  calculator sub <a> <b>            - Subtract (a - b)\n");
    printf("  calculator mul <a> <b>            - Multiply\n");
    printf("  calculator div <a> <b>            - Divide (a / b)\n");
    printf("  calculator result                 - Get last result\n");
    printf("  calculator history                - Get operation history\n");
    printf("\n");
    printf("LED (0x%02X):\n", g_led_addr);
    printf("  led set_all <mask>                - Set all LEDs (e.g., 0x0F)\n");
    printf("  led set_one <idx> <0|1>           - Set single LED (idx 0-3)\n");
    printf("  led blink <idx> <en> <period>     - Blink LED (period in ms)\n");
    printf("  led get_state                     - Get current LED state\n");
    printf("\n");
    printf("Servo (0x%02X):\n", g_servo_addr);
    printf("  servo set_pos <idx> <angle>       - Set position (idx 0-1, angle 0-180)\n");
    printf("  servo set_speed <idx> <speed>     - Set speed (0=instant, 1-20=slow)\n");
    printf("  servo sweep <idx> <en> <min> <max> <step> - Configure sweep\n");
    printf("  servo get_pos                     - Get current positions\n");
    printf("\n");
}

/* ============================================================================
 * Calculator Commands
 * ============================================================================ */

static void cmd_calculator(const char *args)
{
    char subcmd[32];
    int32_t a, b;

    if (sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Error: Missing calculator subcommand\n");
        printf("Usage: calculator <add|sub|mul|div|result|history> [args...]\n");
        return;
    }

    // Arithmetic operations
    if (strcmp(subcmd, "add") == 0)
    {
        if (sscanf(args, "%*s %d %d", &a, &b) != 2)
        {
            printf("Error: add requires two integers\n");
            printf("Usage: calculator add <a> <b>\n");
            return;
        }

        uint8_t msg_buf[32];
        int msg_len = calculator_send_add(&g_ctx, g_calc_addr, a, b, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_calc_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send add command\n");
            return;
        }

        printf("OK: add(%d, %d) command sent\n", a, b);
        printf("    Use 'calculator result' to get the answer\n");
    }
    else if (strcmp(subcmd, "sub") == 0)
    {
        if (sscanf(args, "%*s %d %d", &a, &b) != 2)
        {
            printf("Error: sub requires two integers\n");
            printf("Usage: calculator sub <a> <b>\n");
            return;
        }

        uint8_t msg_buf[32];
        int msg_len = calculator_send_sub(&g_ctx, g_calc_addr, a, b, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_calc_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send sub command\n");
            return;
        }

        printf("OK: sub(%d, %d) command sent\n", a, b);
        printf("    Use 'calculator result' to get the answer\n");
    }
    else if (strcmp(subcmd, "mul") == 0)
    {
        if (sscanf(args, "%*s %d %d", &a, &b) != 2)
        {
            printf("Error: mul requires two integers\n");
            printf("Usage: calculator mul <a> <b>\n");
            return;
        }

        uint8_t msg_buf[32];
        int msg_len = calculator_send_mul(&g_ctx, g_calc_addr, a, b, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_calc_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send mul command\n");
            return;
        }

        printf("OK: mul(%d, %d) command sent\n", a, b);
        printf("    Use 'calculator result' to get the answer\n");
    }
    else if (strcmp(subcmd, "div") == 0)
    {
        if (sscanf(args, "%*s %d %d", &a, &b) != 2)
        {
            printf("Error: div requires two integers\n");
            printf("Usage: calculator div <a> <b>\n");
            return;
        }

        uint8_t msg_buf[32];
        int msg_len = calculator_send_div(&g_ctx, g_calc_addr, a, b, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_calc_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send div command\n");
            return;
        }

        printf("OK: div(%d, %d) command sent\n", a, b);
        printf("    Use 'calculator result' to get the answer\n");
    }
    // Get result
    else if (strcmp(subcmd, "result") == 0)
    {
        uint8_t msg_buf[32];
        int msg_len = calculator_send_get_result(&g_ctx, g_calc_addr, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_calc_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send get result command\n");
            return;
        }

        uint8_t reply_buf[32];
        int reply_len = crumbs_linux_read_message(&g_ctx, &g_lw, g_calc_addr, reply_buf, sizeof(reply_buf), 100);

        if (reply_len < 0)
        {
            printf("Error: Failed to read result\n");
            return;
        }

        int32_t result;
        if (calculator_parse_result_reply(reply_buf, reply_len, &result) < 0)
        {
            printf("Error: Failed to parse result reply\n");
            return;
        }

        printf("Last result: %d\n", result);
    }
    // Get history
    else if (strcmp(subcmd, "history") == 0)
    {
        // First, get metadata
        uint8_t msg_buf[32];
        int msg_len = calculator_send_get_hist_meta(&g_ctx, g_calc_addr, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_calc_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send get history metadata command\n");
            return;
        }

        uint8_t reply_buf[32];
        int reply_len = crumbs_linux_read_message(&g_ctx, &g_lw, g_calc_addr, reply_buf, sizeof(reply_buf), 100);

        if (reply_len < 0)
        {
            printf("Error: Failed to read history metadata\n");
            return;
        }

        uint16_t count, write_pos;
        if (calculator_parse_hist_meta_reply(reply_buf, reply_len, &count, &write_pos) < 0)
        {
            printf("Error: Failed to parse history metadata\n");
            return;
        }

        printf("History: %u entries (write position: %u)\n", count, write_pos);

        // Now get each entry
        for (uint8_t i = 0; i < count; i++)
        {
            msg_len = calculator_send_get_hist_entry(&g_ctx, g_calc_addr, i, msg_buf);

            if (crumbs_linux_i2c_write(&g_lw, g_calc_addr, msg_buf, msg_len) < 0)
            {
                printf("Error: Failed to send get history entry %u command\n", i);
                continue;
            }

            reply_len = crumbs_linux_read_message(&g_ctx, &g_lw, g_calc_addr, reply_buf, sizeof(reply_buf), 100);

            if (reply_len < 0)
            {
                printf("Error: Failed to read history entry %u\n", i);
                continue;
            }

            calc_history_entry_t entry;
            if (calculator_parse_hist_entry_reply(reply_buf, reply_len, &entry) < 0)
            {
                printf("Error: Failed to parse history entry %u\n", i);
                continue;
            }

            // Display the entry
            const char *op_str = "???";
            switch (entry.operation)
            {
            case CALC_OP_ADD:
                op_str = "ADD";
                break;
            case CALC_OP_SUB:
                op_str = "SUB";
                break;
            case CALC_OP_MUL:
                op_str = "MUL";
                break;
            case CALC_OP_DIV:
                op_str = "DIV";
                break;
            }

            printf("  [%u] %s (%d, %d) = %d\n",
                   i, op_str, entry.operand_a, entry.operand_b, entry.result);
        }
    }
    else
    {
        printf("Error: Unknown calculator subcommand: %s\n", subcmd);
        printf("Valid subcommands: add, sub, mul, div, result, history\n");
    }
}

/* ============================================================================
 * LED Commands
 * ============================================================================ */

static void cmd_led(const char *args)
{
    char subcmd[32];

    if (sscanf(args, "%31s", subcmd) != 1)
    {
        printf("Error: Missing LED subcommand\n");
        printf("Usage: led <set_all|set_one|blink|get_state> [args...]\n");
        return;
    }

    if (strcmp(subcmd, "set_all") == 0)
    {
        int mask;
        if (sscanf(args, "%*s %i", &mask) != 1)
        {
            printf("Error: set_all requires bitmask (0-15 or 0x0-0xF)\n");
            printf("Usage: led set_all <mask>\n");
            return;
        }

        uint8_t msg_buf[32];
        int msg_len = led_send_set_all(&g_ctx, g_led_addr, (uint8_t)mask, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_led_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send set_all command\n");
            return;
        }

        printf("OK: All LEDs set to 0x%02X\n", mask);
    }
    else if (strcmp(subcmd, "set_one") == 0)
    {
        int idx, state;
        if (sscanf(args, "%*s %d %d", &idx, &state) != 2)
        {
            printf("Error: set_one requires index (0-3) and state (0 or 1)\n");
            printf("Usage: led set_one <idx> <0|1>\n");
            return;
        }

        if (idx < 0 || idx > 3)
        {
            printf("Error: LED index must be 0-3\n");
            return;
        }

        uint8_t msg_buf[32];
        int msg_len = led_send_set_one(&g_ctx, g_led_addr, (uint8_t)idx, (uint8_t)state, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_led_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send set_one command\n");
            return;
        }

        printf("OK: LED %d set to %s\n", idx, state ? "ON" : "OFF");
    }
    else if (strcmp(subcmd, "blink") == 0)
    {
        int idx, enable;
        uint16_t period_ms;
        if (sscanf(args, "%*s %d %d %hu", &idx, &enable, &period_ms) != 3)
        {
            printf("Error: blink requires index (0-3), enable (0 or 1), and period (ms)\n");
            printf("Usage: led blink <idx> <en> <ms>\n");
            return;
        }

        if (idx < 0 || idx > 3)
        {
            printf("Error: LED index must be 0-3\n");
            return;
        }

        uint8_t msg_buf[32];
        int msg_len = led_send_blink(&g_ctx, g_led_addr, (uint8_t)idx, (uint8_t)enable, period_ms, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_led_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send blink command\n");
            return;
        }

        if (enable)
        {
            printf("OK: LED %d blinking with %u ms period\n", idx, period_ms);
        }
        else
        {
            printf("OK: LED %d blink disabled\n", idx);
        }
    }
    else if (strcmp(subcmd, "get_state") == 0)
    {
        uint8_t msg_buf[32];
        int msg_len = led_send_get_state(&g_ctx, g_led_addr, msg_buf);

        if (crumbs_linux_i2c_write(&g_lw, g_led_addr, msg_buf, msg_len) < 0)
        {
            printf("Error: Failed to send get_state command\n");
            return;
        }

        uint8_t reply_buf[32];
        int reply_len = crumbs_linux_read_message(&g_ctx, &g_lw, g_led_addr, reply_buf, sizeof(reply_buf), 100);

        if (reply_len < 0)
        {
            printf("Error: Failed to read LED state\n");
            return;
        }

        uint8_t state;
        if (led_parse_state_reply(reply_buf, reply_len, &state) < 0)
        {
            printf("Error: Failed to parse LED state reply\n");
            return;
        }

        print_led_state_binary(state);
    }
    else
    {
        printf("Error: Unknown LED subcommand: %s\n", subcmd);
        printf("Valid subcommands: set_all, set_one, blink, get_state\n");
    }
}

/*
 * ============================================================================
 * Servo Commands
 * ============================================================================ */

static void cmd_servo(const char *args)

    if (sscanf(args, "%31s", subcmd) != 1)
{
    printf("Error: Missing servo subcommand\n");
    printf("Usage: servo <set_pos|set_speed|sweep|get_pos> [args...]\n");
    return;
}

if (strcmp(subcmd, "set_pos") == 0)
{
    int idx, angle;
    if (sscanf(args, "%*s %d %d", &idx, &angle) != 2)
    {
        printf("Error: set_pos requires index (0-1) and angle (0-180)\n");
        printf("Usage: servo set_pos <idx> <angle>\n");
        return;
    }

    if (idx < 0 || idx > 1)
    {
        printf("Error: Servo index must be 0-1\n");
        return;
    }

    if (angle < 0 || angle > 180)
    {
        printf("Error: Angle must be 0-180 degrees\n");
        return;
    }

    uint8_t msg_buf[32];
    int msg_len = servo_send_set_pos(&g_ctx, g_servo_addr, (uint8_t)idx, (uint8_t)angle, msg_buf);

    if (crumbs_linux_i2c_write(&g_lw, g_servo_addr, msg_buf, msg_len) < 0)
    {
        printf("Error: Failed to send set_pos command\n");
        return;
    }

    printf("OK: Servo %d set to %d degrees\n", idx, angle);
}
else if (strcmp(subcmd, "set_speed") == 0)
{
    int idx, speed;
    if (sscanf(args, "%*s %d %d", &idx, &speed) != 2)
    {
        printf("Error: set_speed requires index (0-1) and speed (0-20)\n");
        printf("Usage: servo set_speed <idx> <speed>\n");
        return;
    }

    if (idx < 0 || idx > 1)
    {
        printf("Error: Servo index must be 0-1\n");
        return;
    }

    if (speed < 0 || speed > 20)
    {
        printf("Error: Speed must be 0-20 (0=instant, higher=slower)\n");
        return;
    }

    uint8_t msg_buf[32];
    int msg_len = servo_send_set_speed(&g_ctx, g_servo_addr, (uint8_t)idx, (uint8_t)speed, msg_buf);

    if (crumbs_linux_i2c_write(&g_lw, g_servo_addr, msg_buf, msg_len) < 0)
    {
        printf("Error: Failed to send set_speed command\n");
        return;
    }

    printf("OK: Servo %d speed set to %d (0=instant, higher=slower)\n", idx, speed);
}
else if (strcmp(subcmd, "sweep") == 0)
{
    int idx, enable, min_angle, max_angle, step;
    if (sscanf(args, "%*s %d %d %d %d %d", &idx, &enable, &min_angle, &max_angle, &step) != 5)
    {
        printf("Error: sweep requires index, enable, min, max, step\n");
        printf("Usage: servo sweep <idx> <en> <min> <max> <step>\n");
        return;
    }

    if (idx < 0 || idx > 1)
    {
        printf("Error: Servo index must be 0-1\n");
        return;
    }

    uint8_t msg_buf[32];
    int msg_len = servo_send_sweep(&g_ctx, g_servo_addr, (uint8_t)idx, (uint8_t)enable,
                                   (uint8_t)min_angle, (uint8_t)max_angle, (uint8_t)step, msg_buf);

    if (crumbs_linux_i2c_write(&g_lw, g_servo_addr, msg_buf, msg_len) < 0)
    {
        printf("Error: Failed to send sweep command\n");
        return;
    }

    if (enable)
    {
        printf("OK: Servo %d sweeping from %d to %d degrees (step %d)\n",
               idx, min_angle, max_angle, step);
    }
    else
    {
        printf("OK: Servo %d sweep disabled\n", idx);
    }
}
else if (strcmp(subcmd, "get_pos") == 0)
{
    uint8_t msg_buf[32];
    int msg_len = servo_send_get_pos(&g_ctx, g_servo_addr, msg_buf);

    if (crumbs_linux_i2c_write(&g_lw, g_servo_addr, msg_buf, msg_len) < 0)
    {
        printf("Error: Failed to send get_pos command\n");
        return;
    }

    uint8_t reply_buf[32];
    int reply_len = crumbs_linux_read_message(&g_ctx, &g_lw, g_servo_addr, reply_buf, sizeof(reply_buf), 100);

    if (reply_len < 0)
    {
        printf("Error: Failed to read servo positions\n");
        return;
    }

    uint8_t pos0, pos1;
    if (servo_parse_pos_reply(reply_buf, reply_len, &pos0, &pos1) < 0)
    {
        printf("Error: Failed to parse servo position reply\n");
        return;
    }

    printf("Servo positions: Servo 0 = %u°, Servo 1 = %u°\n", pos0, pos1);
}
else
{
    printf("Error: Unknown servo subcommand: %s\n", subcmd);
    printf("Valid subcommands: set_pos, set_speed, sweep, get_pos\n");
}
}

/*
 * ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char *argv[])
{
    const char *i2c_device = DEFAULT_I2C_BUS_PATH;

    if (argc > 1)
        i2c_device = argv[1];

    crumbs_init(&g_ctx);

    if (crumbs_linux_init_controller(&g_lw, i2c_device, 100) < 0)
    {
        fprintf(stderr, "Error: Failed to open I2C device: %s\n", i2c_device);
        fprintf(stderr, "       Check permissions: sudo chmod 666 %s\n", i2c_device);
        return 1;
    }

    printf("\n");
    printf("LHWIT Manual Controller\n");
    printf("=======================\n");
    printf("\n");
    printf("I2C Device: %s\n", i2c_device);
    printf("\n");
    printf("Configured Addresses (from config.h):\n");
    printf("  Calculator: 0x%02X  |  LED: 0x%02X  |  Servo: 0x%02X\n",
           g_calc_addr, g_led_addr, g_servo_addr);
    printf("\n");
    printf("Ready! Type 'help' for command reference.\n");
    printf("\n");

    char line[256];
    while (1)
    {
        printf("lhwit> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin))
            break;

        trim_whitespace(line);

        if (strlen(line) == 0)
            continue;

        char cmd[32];
        if (sscanf(line, "%31s", cmd) != 1)
            continue;

        // Dispatch commands
        if (strcmp(cmd, "help") == 0)
        {
            print_help();
        }
        else if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0)
        {
            printf("Goodbye!\n");
            break;
        }
        else if (strcmp(cmd, "calculator") == 0)
        {
            const char *args = line + strlen("calculator");
            cmd_calculator(args);
        }
        else if (strcmp(cmd, "led") == 0)
        {
            const char *args = line + strlen("led");
            cmd_led(args);
        }
        else if (strcmp(cmd, "servo") == 0)
        {
            const char *args = line + strlen("servo");
            cmd_servo(args);
        }
        else
        {
            printf("Error: Unknown command: %s\n", cmd);
            printf("Type 'help' for available commands\n");
        }
    }

    crumbs_linux_close(&g_lw);
    return 0;
}
