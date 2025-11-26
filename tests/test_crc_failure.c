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
    m.data[0] = 1.234f;

    uint8_t frame[CRUMBS_MESSAGE_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));
    if (w != CRUMBS_MESSAGE_SIZE)
    {
        fprintf(stderr, "encode length mismatch\n");
        return 2;
    }

    /* Corrupt one byte in the frame payload so CRC no longer matches */
    frame[5] ^= 0xFF;

    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(frame, (size_t)w, &out, &ctx);
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
