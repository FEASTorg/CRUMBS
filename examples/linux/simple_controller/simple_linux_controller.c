#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"
#include "crumbs_linux.h" // The Linux HAL you just created

// Address of your slice peripheral (match your Arduino example)
#define SLICE_ADDR 0x08

int main(void)
{
    printf("CRUMBS Linux Controller Example\n");

    crumbs_context_t ctx;
    crumbs_linux_i2c_t lw;

    //----------------------------------------------------------------------
    // Initialize the controller on /dev/i2c-1
    //----------------------------------------------------------------------
    int rc = crumbs_linux_init_controller(&ctx, &lw, "/dev/i2c-1", 25000);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR: crumbs_linux_init_controller failed (%d)\n", rc);
        return 1;
    }

    //----------------------------------------------------------------------
    // Build a message to send to the Arduino slice
    //----------------------------------------------------------------------
    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));

    msg.type_id = 1;
    msg.command_type = 1;

    // Example data payload
    msg.data[0] = 12.34f;
    msg.data[1] = 5.0f;
    msg.data[2] = 9.87f;
    msg.data[3] = 0.0f;
    msg.data[4] = 0.0f;
    msg.data[5] = 0.0f;
    msg.data[6] = 0.0f;

    //----------------------------------------------------------------------
    // Send message to slice
    //----------------------------------------------------------------------
    printf("Sending message to slice (0x%02X)...\n", SLICE_ADDR);

    rc = crumbs_controller_send(
        &ctx,
        SLICE_ADDR,
        &msg,
        crumbs_linux_i2c_write, // Linux HAL write adapter
        &lw                     // Linux I2C context
    );

    if (rc != 0)
    {
        fprintf(stderr, "ERROR: crumbs_controller_send failed (%d)\n", rc);
        crumbs_linux_close(&lw);
        return 1;
    }

    printf("Message sent.\n");

    //----------------------------------------------------------------------
    // Request a reply (Arduino slice will respond in onRequest handler)
    //----------------------------------------------------------------------
    printf("Requesting reply from slice...\n");

    crumbs_message_t reply;
    rc = crumbs_linux_read_message(&lw, SLICE_ADDR, &ctx, &reply);

    if (rc != 0)
    {
        fprintf(stderr, "ERROR: Failed to read reply (%d)\n", rc);
        crumbs_linux_close(&lw);
        return 1;
    }

    //----------------------------------------------------------------------
    // Print reply
    //----------------------------------------------------------------------
    printf("Reply received:\n");
    printf("  type_id:       %u\n", reply.type_id);
    printf("  command_type:  %u\n", reply.command_type);
    printf("  data:          ");

    for (int i = 0; i < CRUMBS_DATA_LENGTH; i++)
    {
        printf("%f ", reply.data[i]);
    }

    printf("\n  crc8:          0x%02X\n", reply.crc8);

    printf("\nCRC Stats:\n");
    printf("  crc_error_count: %u\n", crumbs_get_crc_error_count(&ctx));
    printf("  last_crc_ok:     %d\n", crumbs_last_crc_ok(&ctx));

    //----------------------------------------------------------------------
    // Shutdown
    //----------------------------------------------------------------------
    crumbs_linux_close(&lw);
    printf("\nDone.\n");
    return 0;
}
