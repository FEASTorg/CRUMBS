/*
 * Lightweight unit test for encode/decode and CRC correctness.
 * - Builds a message
 * - Encodes it into a buffer
 * - Decodes it back and verifies fields and CRC status
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
    m.type_id = 0xAA;
    m.command_type = 0x55;
    for (size_t i = 0; i < CRUMBS_DATA_LENGTH; ++i)
        m.data[i] = (float)i + 1.5f;

    uint8_t buf[CRUMBS_MESSAGE_SIZE];
    size_t w = crumbs_encode_message(&m, buf, sizeof(buf));
    if (w != CRUMBS_MESSAGE_SIZE)
    {
        fprintf(stderr, "encode length mismatch: got %zu expected %u\n", w, (unsigned)CRUMBS_MESSAGE_SIZE);
        return 2;
    }

    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(buf, (size_t)w, &out, &ctx);
    if (rc != 0)
    {
        fprintf(stderr, "decode failed (rc=%d)\n", rc);
        return 3;
    }

    if (out.type_id != m.type_id || out.command_type != m.command_type)
    {
        fprintf(stderr, "decoded header mismatch\n");
        return 4;
    }

    for (size_t i = 0; i < CRUMBS_DATA_LENGTH; ++i)
    {
        if (out.data[i] != m.data[i])
        {
            fprintf(stderr, "data mismatch at %zu\n", i);
            return 5;
        }
    }

    if (!crumbs_last_crc_ok(&ctx))
    {
        fprintf(stderr, "CRC flagged bad unexpectedly\n");
        return 6;
    }

    printf("OK encode/decode test\n");
    return 0;
}
