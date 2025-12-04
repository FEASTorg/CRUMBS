/*
 * Lightweight unit test for encode/decode and CRC correctness.
 * Tests variable-length payload encoding with various data sizes.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"

static int test_basic_encode_decode(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0xAA;
    m.command_type = 0x55;
    m.data_len = 5;
    for (size_t i = 0; i < m.data_len; ++i)
        m.data[i] = (uint8_t)(i + 1);

    uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, buf, sizeof(buf));

    /* Expected: 4 + data_len = 9 bytes */
    size_t expected_len = 4 + m.data_len;
    if (w != expected_len)
    {
        fprintf(stderr, "encode length mismatch: got %zu expected %zu\n", w, expected_len);
        return 1;
    }

    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(buf, w, &out, &ctx);
    if (rc != 0)
    {
        fprintf(stderr, "decode failed (rc=%d)\n", rc);
        return 1;
    }

    if (out.type_id != m.type_id || out.command_type != m.command_type)
    {
        fprintf(stderr, "decoded header mismatch\n");
        return 1;
    }

    if (out.data_len != m.data_len)
    {
        fprintf(stderr, "decoded data_len mismatch: got %u expected %u\n", out.data_len, m.data_len);
        return 1;
    }

    for (size_t i = 0; i < m.data_len; ++i)
    {
        if (out.data[i] != m.data[i])
        {
            fprintf(stderr, "data mismatch at %zu\n", i);
            return 1;
        }
    }

    if (!crumbs_last_crc_ok(&ctx))
    {
        fprintf(stderr, "CRC flagged bad unexpectedly\n");
        return 1;
    }

    printf("  basic encode/decode: PASS\n");
    return 0;
}

static int test_zero_length_payload(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x01;
    m.command_type = 0x02;
    m.data_len = 0; /* zero-length payload */

    uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, buf, sizeof(buf));

    /* Expected: 4 bytes (header + crc) */
    if (w != 4)
    {
        fprintf(stderr, "zero-length encode: got %zu expected 4\n", w);
        return 1;
    }

    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(buf, w, &out, &ctx);
    if (rc != 0)
    {
        fprintf(stderr, "zero-length decode failed (rc=%d)\n", rc);
        return 1;
    }

    if (out.data_len != 0)
    {
        fprintf(stderr, "zero-length: data_len mismatch\n");
        return 1;
    }

    printf("  zero-length payload: PASS\n");
    return 0;
}

static int test_max_length_payload(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0xFF;
    m.command_type = 0xFE;
    m.data_len = CRUMBS_MAX_PAYLOAD; /* 27 bytes */
    for (size_t i = 0; i < CRUMBS_MAX_PAYLOAD; ++i)
        m.data[i] = (uint8_t)(i ^ 0xAA);

    uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, buf, sizeof(buf));

    /* Expected: 4 + 27 = 31 bytes (max frame) */
    if (w != CRUMBS_MESSAGE_MAX_SIZE)
    {
        fprintf(stderr, "max-length encode: got %zu expected %u\n", w, (unsigned)CRUMBS_MESSAGE_MAX_SIZE);
        return 1;
    }

    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(buf, w, &out, &ctx);
    if (rc != 0)
    {
        fprintf(stderr, "max-length decode failed (rc=%d)\n", rc);
        return 1;
    }

    if (out.data_len != CRUMBS_MAX_PAYLOAD)
    {
        fprintf(stderr, "max-length: data_len mismatch\n");
        return 1;
    }

    for (size_t i = 0; i < CRUMBS_MAX_PAYLOAD; ++i)
    {
        if (out.data[i] != m.data[i])
        {
            fprintf(stderr, "max-length: data mismatch at %zu\n", i);
            return 1;
        }
    }

    printf("  max-length payload: PASS\n");
    return 0;
}

static int test_oversized_data_len(void)
{
    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x01;
    m.command_type = 0x02;
    m.data_len = CRUMBS_MAX_PAYLOAD + 1; /* invalid */

    uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE + 10];
    size_t w = crumbs_encode_message(&m, buf, sizeof(buf));

    if (w != 0)
    {
        fprintf(stderr, "oversized data_len should fail encode, got %zu\n", w);
        return 1;
    }

    printf("  oversized data_len rejection: PASS\n");
    return 0;
}

static int test_truncated_frame(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    /* Build a valid frame first */
    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x10;
    m.command_type = 0x20;
    m.data_len = 5;
    for (size_t i = 0; i < 5; ++i)
        m.data[i] = (uint8_t)i;

    uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, buf, sizeof(buf));

    /* Try to decode with less bytes than needed (truncated) */
    crumbs_message_t out;
    memset(&out, 0, sizeof(out));

    int rc = crumbs_decode_message(buf, w - 2, &out, &ctx);
    if (rc == 0)
    {
        fprintf(stderr, "truncated frame should fail decode\n");
        return 1;
    }

    printf("  truncated frame rejection: PASS\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("Running encode/decode tests:\n");

    failures += test_basic_encode_decode();
    failures += test_zero_length_payload();
    failures += test_max_length_payload();
    failures += test_oversized_data_len();
    failures += test_truncated_frame();

    if (failures == 0)
    {
        printf("OK all encode/decode tests passed\n");
        return 0;
    }
    else
    {
        fprintf(stderr, "FAILED %d test(s)\n", failures);
        return 1;
    }
}
