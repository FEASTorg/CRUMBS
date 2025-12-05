/**
 * @file multi_controller.c
 * @brief Example Linux controller using command interface for multiple devices.
 *
 * This example demonstrates:
 * - Using led_commands.h and servo_commands.h for type-safe commands
 * - Controlling multiple device types from a single controller
 * - Reading responses from peripherals
 *
 * Hardware Setup:
 * - Linux SBC (Raspberry Pi, etc.) as controller
 * - LED peripheral at I2C address 0x08
 * - Servo peripheral at I2C address 0x09
 *
 * Build:
 *   gcc -I../../.. -I../../../src multi_controller.c \
 *       ../../../src/core/crumbs_core.c \
 *       ../../../src/hal/linux/crumbs_i2c_linux.c \
 *       ../../../src/crc/crumbs_crc8.c \
 *       -o multi_controller
 *
 * Usage:
 *   ./multi_controller [i2c-device]
 *   ./multi_controller /dev/i2c-1
 */

/* Enable usleep() on glibc - must be before any includes */
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "crumbs.h"
#include "crumbs_linux.h"
#include "crumbs_msg.h"

/* Command definitions */
#include "led_commands.h"
#include "servo_commands.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define LED_ADDR   0x08
#define SERVO_ADDR 0x09

/* ============================================================================
 * Demo Functions
 * ============================================================================ */

/**
 * @brief Demonstrate LED commands.
 */
static void demo_led(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw)
{
    int rc;
    
    printf("\n=== LED Demo ===\n");
    
    /* Turn all LEDs on */
    printf("Setting all LEDs ON (0x0F)...\n");
    rc = led_send_set_all(ctx, LED_ADDR, crumbs_linux_i2c_write, lw, 0x0F);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: led_send_set_all failed (%d)\n", rc);
        return;
    }
    sleep(1);
    
    /* Turn all LEDs off */
    printf("Setting all LEDs OFF (0x00)...\n");
    rc = led_send_set_all(ctx, LED_ADDR, crumbs_linux_i2c_write, lw, 0x00);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: led_send_set_all failed (%d)\n", rc);
        return;
    }
    sleep(1);
    
    /* Turn on LEDs one by one */
    printf("Turning on LEDs one by one...\n");
    for (int i = 0; i < 4; i++) {
        printf("  LED %d ON\n", i);
        rc = led_send_set_one(ctx, LED_ADDR, crumbs_linux_i2c_write, lw, i, 1);
        if (rc != 0) {
            fprintf(stderr, "  ERROR: led_send_set_one failed (%d)\n", rc);
            return;
        }
        usleep(300000);  /* 300ms */
    }
    sleep(1);
    
    /* Blink all LEDs */
    printf("Blinking all LEDs (3 times, 200ms delay)...\n");
    rc = led_send_blink(ctx, LED_ADDR, crumbs_linux_i2c_write, lw, 3, 20);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: led_send_blink failed (%d)\n", rc);
        return;
    }
    sleep(2);  /* Wait for blink to complete */
    
    /* Get current state */
    printf("Requesting LED state...\n");
    rc = led_send_get_state(ctx, LED_ADDR, crumbs_linux_i2c_write, lw);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: led_send_get_state failed (%d)\n", rc);
        return;
    }
    
    /* Read response */
    crumbs_message_t reply;
    rc = crumbs_linux_read_message(lw, LED_ADDR, ctx, &reply);
    if (rc == 0) {
        uint8_t state;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &state) == 0) {
            printf("  LED state: 0x%02X (binary: ", state);
            for (int i = 3; i >= 0; i--) {
                printf("%d", (state >> i) & 1);
            }
            printf(")\n");
        }
    } else {
        fprintf(stderr, "  ERROR: Failed to read LED state (%d)\n", rc);
    }
}

/**
 * @brief Demonstrate servo commands.
 */
static void demo_servo(crumbs_context_t *ctx, crumbs_linux_i2c_t *lw)
{
    int rc;
    
    printf("\n=== Servo Demo ===\n");
    
    /* Center all servos */
    printf("Centering all servos...\n");
    rc = servo_send_center_all(ctx, SERVO_ADDR, crumbs_linux_i2c_write, lw);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: servo_send_center_all failed (%d)\n", rc);
        return;
    }
    sleep(1);
    
    /* Set individual angles */
    printf("Setting servo 0 to 45°...\n");
    rc = servo_send_angle(ctx, SERVO_ADDR, crumbs_linux_i2c_write, lw, 0, 45);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: servo_send_angle failed (%d)\n", rc);
        return;
    }
    sleep(1);
    
    printf("Setting servo 1 to 135°...\n");
    rc = servo_send_angle(ctx, SERVO_ADDR, crumbs_linux_i2c_write, lw, 1, 135);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: servo_send_angle failed (%d)\n", rc);
        return;
    }
    sleep(1);
    
    /* Set both at once */
    printf("Setting both servos to 60°, 120°...\n");
    rc = servo_send_both(ctx, SERVO_ADDR, crumbs_linux_i2c_write, lw, 60, 120);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: servo_send_both failed (%d)\n", rc);
        return;
    }
    sleep(1);
    
    /* Sweep servo 0 */
    printf("Sweeping servo 0 from 0° to 180° (10ms/step)...\n");
    rc = servo_send_sweep(ctx, SERVO_ADDR, crumbs_linux_i2c_write, lw, 0, 0, 180, 10);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: servo_send_sweep failed (%d)\n", rc);
        return;
    }
    sleep(3);  /* Wait for sweep to complete */
    
    /* Get current angles */
    printf("Requesting servo angles...\n");
    rc = servo_send_get_angles(ctx, SERVO_ADDR, crumbs_linux_i2c_write, lw);
    if (rc != 0) {
        fprintf(stderr, "  ERROR: servo_send_get_angles failed (%d)\n", rc);
        return;
    }
    
    /* Read response */
    crumbs_message_t reply;
    rc = crumbs_linux_read_message(lw, SERVO_ADDR, ctx, &reply);
    if (rc == 0) {
        uint8_t angle0, angle1;
        if (crumbs_msg_read_u8(reply.data, reply.data_len, 0, &angle0) == 0 &&
            crumbs_msg_read_u8(reply.data, reply.data_len, 1, &angle1) == 0) {
            printf("  Servo angles: %d°, %d°\n", angle0, angle1);
        }
    } else {
        fprintf(stderr, "  ERROR: Failed to read servo angles (%d)\n", rc);
    }
    
    /* Return to center */
    printf("Returning to center...\n");
    servo_send_center_all(ctx, SERVO_ADDR, crumbs_linux_i2c_write, lw);
}

/* ============================================================================
 * Main
 * ============================================================================ */

int main(int argc, char **argv)
{
    printf("CRUMBS Multi-Device Controller Example\n");
    printf("======================================\n");
    
    crumbs_context_t ctx;
    crumbs_linux_i2c_t lw;
    
    /* Get I2C device path */
    const char *device_path = "/dev/i2c-1";
    if (argc >= 2 && argv[1] && argv[1][0] != '\0') {
        device_path = argv[1];
    }
    
    printf("I2C Device: %s\n", device_path);
    printf("LED Peripheral: 0x%02X\n", LED_ADDR);
    printf("Servo Peripheral: 0x%02X\n", SERVO_ADDR);
    
    /* Initialize controller */
    int rc = crumbs_linux_init_controller(&ctx, &lw, device_path, 25000);
    if (rc != 0) {
        fprintf(stderr, "ERROR: crumbs_linux_init_controller failed (%d)\n", rc);
        fprintf(stderr, "Make sure I2C device exists and you have permissions.\n");
        return 1;
    }
    
    /* Run demos */
    demo_led(&ctx, &lw);
    demo_servo(&ctx, &lw);
    
    /* Cleanup */
    crumbs_linux_close(&lw);
    
    printf("\nDone.\n");
    return 0;
}
