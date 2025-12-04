/*
 * Test that a CRC failure in a CRUMBS frame is detected by the decoder.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"

int main(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x99;
    m.command_type = 0x42;
    m.data_len = 4;
    m.data[0] = 0x12;
    m.data[1] = 0x34;
    m.data[2] = 0x56;
    m.data[3] = 0x78;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));
    if (w != 4 + m.data_len)
    {
        fprintf(stderr, "encode length mismatch\n");
        return 2;
    }

    /* Corrupt one byte in the frame payload so CRC no longer matches */
    frame[4] ^= 0xFF;

    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(frame, w, &out, &ctx);
    /* expect a negative error indicating bad CRC/invalid frame */
    if (rc == 0)
    {
        fprintf(stderr, "decode unexpectedly succeeded despite CRC corruption\n");
        return 3;
    }

    /* ensure last CRC check is reported as failed */
    if (crumbs_last_crc_ok(&ctx))
    {
        fprintf(stderr, "CRC reported OK despite corruption\n");
        return 4;
    }

    printf("OK crc-failure test\n");
    return 0;
}
