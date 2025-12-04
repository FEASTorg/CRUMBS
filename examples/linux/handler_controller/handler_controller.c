/**
 * @file handler_controller.c
 * @brief CRUMBS Linux controller example for handler_peripheral_example.
 *
 * Demonstrates sending commands to a CRUMBS peripheral that uses
 * per-command handlers. This controller sends LED control and echo
 * commands, matching the handler_peripheral_example.ino sketch.
 *
 * Usage:
 *   ./handler_controller [device] [address] [command] [args...]
 *
 * Commands:
 *   on           - Turn LED on  (cmd 0x01)
 *   off          - Turn LED off (cmd 0x02)
 *   blink N D    - Blink N times with D*10ms delay (cmd 0x03)
 *   echo <bytes> - Echo bytes back (cmd 0x10), then read reply
 *
 * Examples:
 *   ./handler_controller /dev/i2c-1 0x08 on
 *   ./handler_controller /dev/i2c-1 0x08 blink 5 10
 *   ./handler_controller /dev/i2c-1 0x08 echo 0x48 0x65 0x6C 0x6C 0x6F
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"
#include "crumbs_linux.h"

/* Command types matching handler_peripheral_example */
#define CMD_LED_ON   0x01
#define CMD_LED_OFF  0x02
#define CMD_BLINK    0x03
#define CMD_ECHO     0x10

static void print_usage(const char *prog)
{
    printf("Usage: %s [device] [address] [command] [args...]\n\n", prog);
    printf("Commands:\n");
    printf("  on           - Turn LED on\n");
    printf("  off          - Turn LED off\n");
    printf("  blink N D    - Blink N times, D*10ms delay\n");
    printf("  echo <bytes> - Echo bytes (hex values)\n\n");
    printf("Example:\n");
    printf("  %s /dev/i2c-1 0x08 blink 3 20\n", prog);
}

static int send_simple_command(crumbs_linux_i2c_t *lw, crumbs_context_t *ctx,
                               uint8_t addr, uint8_t cmd)
{
    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type_id = 1;
    msg.command_type = cmd;
    msg.data_len = 0;

    return crumbs_controller_send(ctx, addr, &msg, crumbs_linux_i2c_write, lw);
}

static int send_blink(crumbs_linux_i2c_t *lw, crumbs_context_t *ctx,
                      uint8_t addr, uint8_t count, uint8_t delay_10ms)
{
    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type_id = 1;
    msg.command_type = CMD_BLINK;
    msg.data_len = 2;
    msg.data[0] = count;
    msg.data[1] = delay_10ms;

    return crumbs_controller_send(ctx, addr, &msg, crumbs_linux_i2c_write, lw);
}

static int send_echo(crumbs_linux_i2c_t *lw, crumbs_context_t *ctx,
                     uint8_t addr, uint8_t *data, uint8_t len)
{
    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type_id = 1;
    msg.command_type = CMD_ECHO;
    msg.data_len = len;
    if (len > 0)
    {
        memcpy(msg.data, data, len);
    }

    int rc = crumbs_controller_send(ctx, addr, &msg, crumbs_linux_i2c_write, lw);
    if (rc != 0)
    {
        return rc;
    }

    /* Read echo reply */
    crumbs_message_t reply;
    rc = crumbs_linux_read_message(lw, addr, ctx, &reply);
    if (rc != 0)
    {
        fprintf(stderr, "Failed to read echo reply: %d\n", rc);
        return rc;
    }

    printf("Echo reply (%u bytes): ", reply.data_len);
    for (int i = 0; i < reply.data_len; i++)
    {
        printf("0x%02X ", reply.data[i]);
    }
    printf("\n");

    /* Try to print as string if printable */
    int printable = 1;
    for (int i = 0; i < reply.data_len; i++)
    {
        if (reply.data[i] < 0x20 || reply.data[i] > 0x7E)
        {
            printable = 0;
            break;
        }
    }
    if (printable && reply.data_len > 0)
    {
        printf("As string: \"%.*s\"\n", reply.data_len, reply.data);
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        print_usage(argv[0]);
        return 1;
    }

    const char *device = argv[1];
    uint8_t addr = (uint8_t)strtoul(argv[2], NULL, 0);
    const char *command = argv[3];

    crumbs_context_t ctx;
    crumbs_linux_i2c_t lw;

    int rc = crumbs_linux_init_controller(&ctx, &lw, device, 25000);
    if (rc != 0)
    {
        fprintf(stderr, "Failed to init controller on %s: %d\n", device, rc);
        return 1;
    }

    printf("CRUMBS Handler Controller\n");
    printf("Device: %s, Address: 0x%02X\n\n", device, addr);

    if (strcmp(command, "on") == 0)
    {
        printf("Sending LED ON...\n");
        rc = send_simple_command(&lw, &ctx, addr, CMD_LED_ON);
    }
    else if (strcmp(command, "off") == 0)
    {
        printf("Sending LED OFF...\n");
        rc = send_simple_command(&lw, &ctx, addr, CMD_LED_OFF);
    }
    else if (strcmp(command, "blink") == 0)
    {
        if (argc < 6)
        {
            fprintf(stderr, "blink requires: count delay\n");
            crumbs_linux_close(&lw);
            return 1;
        }
        uint8_t count = (uint8_t)atoi(argv[4]);
        uint8_t delay_10ms = (uint8_t)atoi(argv[5]);
        printf("Sending BLINK count=%u delay=%ums...\n", count, delay_10ms * 10);
        rc = send_blink(&lw, &ctx, addr, count, delay_10ms);
    }
    else if (strcmp(command, "echo") == 0)
    {
        uint8_t data[CRUMBS_MAX_PAYLOAD];
        uint8_t len = 0;
        for (int i = 4; i < argc && len < CRUMBS_MAX_PAYLOAD; i++)
        {
            data[len++] = (uint8_t)strtoul(argv[i], NULL, 0);
        }
        printf("Sending ECHO with %u bytes...\n", len);
        rc = send_echo(&lw, &ctx, addr, data, len);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_usage(argv[0]);
        crumbs_linux_close(&lw);
        return 1;
    }

    if (rc == 0)
    {
        printf("OK\n");
    }
    else
    {
        fprintf(stderr, "Command failed: %d\n", rc);
    }

    crumbs_linux_close(&lw);
    return rc == 0 ? 0 : 1;
}
