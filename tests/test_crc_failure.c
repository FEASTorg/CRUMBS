/*
 * Test that CRC failures in CRUMBS frames are properly detected by the decoder.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"

static int test_crc_corruption_detected(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x99;
    m.opcode = 0x42;
    m.data_len = 4;
    m.data[0] = 0x12;
    m.data[1] = 0x34;
    m.data[2] = 0x56;
    m.data[3] = 0x78;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));
    if (w != 4 + m.data_len)
    {
        fprintf(stderr, "crc_corruption: encode length mismatch\n");
        return 1;
    }

    /* Corrupt one byte in the frame payload so CRC no longer matches */
    frame[4] ^= 0xFF;

    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(frame, w, &out, &ctx);
    /* Expect a negative error indicating bad CRC/invalid frame */
    if (rc == 0)
    {
        fprintf(stderr, "crc_corruption: decode unexpectedly succeeded\n");
        return 1;
    }

    /* Ensure last CRC check is reported as failed */
    if (crumbs_last_crc_ok(&ctx))
    {
        fprintf(stderr, "crc_corruption: CRC reported OK despite corruption\n");
        return 1;
    }

    printf("  CRC corruption detected: PASS\n");
    return 0;
}

static int test_crc_single_bit_flip(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x01;
    m.opcode = 0x02;
    m.data_len = 2;
    m.data[0] = 0xAA;
    m.data[1] = 0x55;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));

    /* Flip just one bit in the CRC byte itself */
    frame[w - 1] ^= 0x01;

    crumbs_message_t out;
    int rc = crumbs_decode_message(frame, w, &out, &ctx);
    if (rc == 0)
    {
        fprintf(stderr, "crc_single_bit: single bit flip should be detected\n");
        return 1;
    }

    printf("  CRC single bit flip detected: PASS\n");
    return 0;
}

static int test_crc_header_corruption(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x10;
    m.opcode = 0x20;
    m.data_len = 0;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));

    /* Corrupt the type_id byte */
    frame[0] ^= 0x01;

    crumbs_message_t out;
    int rc = crumbs_decode_message(frame, w, &out, &ctx);
    if (rc == 0)
    {
        fprintf(stderr, "crc_header: header corruption should be detected\n");
        return 1;
    }

    printf("  CRC header corruption detected: PASS\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("Running CRC failure tests:\n");

    failures += test_crc_corruption_detected();
    failures += test_crc_single_bit_flip();
    failures += test_crc_header_corruption();

    if (failures == 0)
    {
        printf("OK all CRC failure tests passed\n");
        return 0;
    }
    else
    {
        fprintf(stderr, "FAILED %d test(s)\n", failures);
        return 1;
    }
}
