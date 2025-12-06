/*
 * Unit tests for CRUMBS context initialization, configuration, and CRC statistics.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"

/* ---- Context Initialization Tests ------------------------------------- */

static int test_init_controller(void)
{
    crumbs_context_t ctx;
    memset(&ctx, 0xFF, sizeof(ctx)); /* Fill with garbage */

    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0x42);

    if (ctx.role != CRUMBS_ROLE_CONTROLLER)
    {
        fprintf(stderr, "init_controller: role mismatch\n");
        return 1;
    }

    /* Controller should have address 0 regardless of what was passed */
    if (ctx.address != 0)
    {
        fprintf(stderr, "init_controller: address should be 0\n");
        return 1;
    }

    if (ctx.crc_error_count != 0)
    {
        fprintf(stderr, "init_controller: crc_error_count should be 0\n");
        return 1;
    }

    if (ctx.on_message != NULL || ctx.on_request != NULL)
    {
        fprintf(stderr, "init_controller: callbacks should be NULL\n");
        return 1;
    }

    printf("  init controller: PASS\n");
    return 0;
}

static int test_init_peripheral(void)
{
    crumbs_context_t ctx;
    memset(&ctx, 0xFF, sizeof(ctx));

    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x42);

    if (ctx.role != CRUMBS_ROLE_PERIPHERAL)
    {
        fprintf(stderr, "init_peripheral: role mismatch\n");
        return 1;
    }

    /* Peripheral should store the provided address */
    if (ctx.address != 0x42)
    {
        fprintf(stderr, "init_peripheral: address should be 0x42\n");
        return 1;
    }

    printf("  init peripheral: PASS\n");
    return 0;
}

static int test_init_null_ctx(void)
{
    /* Should not crash with NULL ctx */
    crumbs_init(NULL, CRUMBS_ROLE_CONTROLLER, 0);

    printf("  init NULL ctx: PASS\n");
    return 0;
}

static int test_context_size(void)
{
    size_t lib_size = crumbs_context_size();
    size_t local_size = sizeof(crumbs_context_t);

    if (lib_size != local_size)
    {
        fprintf(stderr, "context_size: mismatch (lib=%zu, local=%zu)\n",
                lib_size, local_size);
        return 1;
    }

    printf("  context_size: PASS\n");
    return 0;
}

/* ---- Callback Configuration Tests ------------------------------------- */

static int g_on_message_called = 0;
static int g_on_request_called = 0;
static void *g_user_data_received = NULL;

static void test_on_message(crumbs_context_t *ctx, const crumbs_message_t *msg)
{
    (void)msg;
    g_on_message_called++;
    g_user_data_received = ctx->user_data;
}

static void test_on_request(crumbs_context_t *ctx, crumbs_message_t *msg)
{
    (void)msg;
    g_on_request_called++;
    g_user_data_received = ctx->user_data;
}

static int test_set_callbacks(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);

    void *user_data = (void *)(uintptr_t)0xDEADBEEF;
    crumbs_set_callbacks(&ctx, test_on_message, test_on_request, user_data);

    if (ctx.on_message != test_on_message)
    {
        fprintf(stderr, "set_callbacks: on_message not set\n");
        return 1;
    }

    if (ctx.on_request != test_on_request)
    {
        fprintf(stderr, "set_callbacks: on_request not set\n");
        return 1;
    }

    if (ctx.user_data != user_data)
    {
        fprintf(stderr, "set_callbacks: user_data not set\n");
        return 1;
    }

    printf("  set_callbacks: PASS\n");
    return 0;
}

static int test_set_callbacks_null(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);

    /* Set callbacks first */
    crumbs_set_callbacks(&ctx, test_on_message, test_on_request, (void *)(uintptr_t)0x1234);

    /* Then clear them */
    crumbs_set_callbacks(&ctx, NULL, NULL, NULL);

    if (ctx.on_message != NULL || ctx.on_request != NULL || ctx.user_data != NULL)
    {
        fprintf(stderr, "set_callbacks_null: callbacks should be NULL\n");
        return 1;
    }

    printf("  set_callbacks NULL: PASS\n");
    return 0;
}

static int test_set_callbacks_null_ctx(void)
{
    /* Should not crash with NULL ctx */
    crumbs_set_callbacks(NULL, test_on_message, test_on_request, (void *)(uintptr_t)0x1234);

    printf("  set_callbacks NULL ctx: PASS\n");
    return 0;
}

/* ---- CRC Statistics Tests --------------------------------------------- */

static int test_crc_error_count_increments(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    /* Build a valid frame */
    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 2;
    msg.data[0] = 0xAA;
    msg.data[1] = 0xBB;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    /* Corrupt the frame */
    frame[3] ^= 0xFF;

    crumbs_message_t out;
    int rc = crumbs_decode_message(frame, len, &out, &ctx);
    if (rc == 0)
    {
        fprintf(stderr, "crc_error_count: decode should fail\n");
        return 1;
    }

    if (crumbs_get_crc_error_count(&ctx) != 1)
    {
        fprintf(stderr, "crc_error_count: should be 1\n");
        return 1;
    }

    /* Corrupt and decode again */
    frame[3] ^= 0x01; /* Still corrupted */
    crumbs_decode_message(frame, len, &out, &ctx);

    if (crumbs_get_crc_error_count(&ctx) != 2)
    {
        fprintf(stderr, "crc_error_count: should be 2\n");
        return 1;
    }

    printf("  crc_error_count increments: PASS\n");
    return 0;
}

static int test_last_crc_ok(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    /* Build a valid frame */
    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 0;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    crumbs_message_t out;
    int rc = crumbs_decode_message(frame, len, &out, &ctx);
    if (rc != 0)
    {
        fprintf(stderr, "last_crc_ok: valid decode failed\n");
        return 1;
    }

    if (!crumbs_last_crc_ok(&ctx))
    {
        fprintf(stderr, "last_crc_ok: should be true after valid decode\n");
        return 1;
    }

    /* Now corrupt and decode */
    frame[2] ^= 0xFF;
    crumbs_decode_message(frame, len, &out, &ctx);

    if (crumbs_last_crc_ok(&ctx))
    {
        fprintf(stderr, "last_crc_ok: should be false after corrupt decode\n");
        return 1;
    }

    printf("  last_crc_ok: PASS\n");
    return 0;
}

static int test_reset_crc_stats(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    /* Simulate some CRC errors by manually setting state */
    ctx.crc_error_count = 5;
    ctx.last_crc_ok = 0;

    crumbs_reset_crc_stats(&ctx);

    if (crumbs_get_crc_error_count(&ctx) != 0)
    {
        fprintf(stderr, "reset_crc_stats: error_count should be 0\n");
        return 1;
    }

    if (!crumbs_last_crc_ok(&ctx))
    {
        fprintf(stderr, "reset_crc_stats: last_crc_ok should be true\n");
        return 1;
    }

    printf("  reset_crc_stats: PASS\n");
    return 0;
}

static int test_crc_stats_null_ctx(void)
{
    /* These should not crash and return sensible defaults */
    uint32_t count = crumbs_get_crc_error_count(NULL);
    if (count != 0)
    {
        fprintf(stderr, "crc_stats_null: get_crc_error_count(NULL) should be 0\n");
        return 1;
    }

    int ok = crumbs_last_crc_ok(NULL);
    if (ok != 0)
    {
        fprintf(stderr, "crc_stats_null: last_crc_ok(NULL) should be 0\n");
        return 1;
    }

    /* Should not crash */
    crumbs_reset_crc_stats(NULL);

    printf("  crc_stats NULL ctx: PASS\n");
    return 0;
}

/* ---- Encode Edge Cases ------------------------------------------------ */

static int test_encode_null_params(void)
{
    crumbs_message_t msg = {0};
    uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE];

    /* NULL msg */
    size_t len = crumbs_encode_message(NULL, buf, sizeof(buf));
    if (len != 0)
    {
        fprintf(stderr, "encode_null: should return 0 for NULL msg\n");
        return 1;
    }

    /* NULL buffer */
    len = crumbs_encode_message(&msg, NULL, sizeof(buf));
    if (len != 0)
    {
        fprintf(stderr, "encode_null: should return 0 for NULL buffer\n");
        return 1;
    }

    printf("  encode NULL params: PASS\n");
    return 0;
}

static int test_encode_buffer_too_small(void)
{
    crumbs_message_t msg = {0};
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 5;

    /* Buffer too small for frame (need 4 + 5 = 9 bytes) */
    uint8_t buf[8];
    size_t len = crumbs_encode_message(&msg, buf, sizeof(buf));
    if (len != 0)
    {
        fprintf(stderr, "encode_buffer_too_small: should return 0\n");
        return 1;
    }

    printf("  encode buffer too small: PASS\n");
    return 0;
}

/* ---- Decode Edge Cases ------------------------------------------------ */

static int test_decode_null_params(void)
{
    uint8_t buf[10] = {0};
    crumbs_message_t msg;

    /* NULL buffer */
    int rc = crumbs_decode_message(NULL, 10, &msg, NULL);
    if (rc != -1)
    {
        fprintf(stderr, "decode_null: should return -1 for NULL buffer\n");
        return 1;
    }

    /* NULL msg */
    rc = crumbs_decode_message(buf, 10, NULL, NULL);
    if (rc != -1)
    {
        fprintf(stderr, "decode_null: should return -1 for NULL msg\n");
        return 1;
    }

    printf("  decode NULL params: PASS\n");
    return 0;
}

static int test_decode_without_ctx(void)
{
    /* Decoding should work without a context (just no stats updated) */
    crumbs_message_t msg = {0};
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 0;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    crumbs_message_t out;
    int rc = crumbs_decode_message(frame, len, &out, NULL);
    if (rc != 0)
    {
        fprintf(stderr, "decode_without_ctx: should succeed\n");
        return 1;
    }

    if (out.type_id != 0x01 || out.command_type != 0x02)
    {
        fprintf(stderr, "decode_without_ctx: header mismatch\n");
        return 1;
    }

    printf("  decode without ctx: PASS\n");
    return 0;
}

/* ---- Controller Role Validation --------------------------------------- */

/* Fake write function for wrong role test */
static int g_fake_write_called = 0;

static int fake_write_for_role_test(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len)
{
    (void)user_ctx;
    (void)addr;
    (void)data;
    (void)len;
    g_fake_write_called = 1;
    return 0;
}

static int test_controller_send_wrong_role(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10); /* Wrong role */

    crumbs_message_t msg = {0};
    msg.type_id = 0x01;
    msg.command_type = 0x02;

    g_fake_write_called = 0;

    int rc = crumbs_controller_send(&ctx, 0x20, &msg, fake_write_for_role_test, NULL);
    if (rc == 0)
    {
        fprintf(stderr, "controller_send_wrong_role: should fail\n");
        return 1;
    }

    if (g_fake_write_called)
    {
        fprintf(stderr, "controller_send_wrong_role: write should not be called\n");
        return 1;
    }

    printf("  controller_send wrong role: PASS\n");
    return 0;
}

static int dummy_write_for_null_test(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len)
{
    (void)user_ctx;
    (void)addr;
    (void)data;
    (void)len;
    return 0;
}

static int test_controller_send_null_params(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t msg = {0};

    /* NULL ctx */
    int rc = crumbs_controller_send(NULL, 0x20, &msg, dummy_write_for_null_test, NULL);
    if (rc == 0)
    {
        fprintf(stderr, "controller_send_null: should fail with NULL ctx\n");
        return 1;
    }

    /* NULL msg */
    rc = crumbs_controller_send(&ctx, 0x20, NULL, dummy_write_for_null_test, NULL);
    if (rc == 0)
    {
        fprintf(stderr, "controller_send_null: should fail with NULL msg\n");
        return 1;
    }

    /* NULL write_fn */
    rc = crumbs_controller_send(&ctx, 0x20, &msg, NULL, NULL);
    if (rc == 0)
    {
        fprintf(stderr, "controller_send_null: should fail with NULL write_fn\n");
        return 1;
    }

    printf("  controller_send NULL params: PASS\n");
    return 0;
}

/* ---- Main ------------------------------------------------------------- */

int main(void)
{
    int failures = 0;

    printf("Running context tests:\n");
    printf("  Initialization:\n");

    failures += test_init_controller();
    failures += test_init_peripheral();
    failures += test_init_null_ctx();
    failures += test_context_size();

    printf("  Callbacks:\n");

    failures += test_set_callbacks();
    failures += test_set_callbacks_null();
    failures += test_set_callbacks_null_ctx();

    printf("  CRC Statistics:\n");

    failures += test_crc_error_count_increments();
    failures += test_last_crc_ok();
    failures += test_reset_crc_stats();
    failures += test_crc_stats_null_ctx();

    printf("  Encode Edge Cases:\n");

    failures += test_encode_null_params();
    failures += test_encode_buffer_too_small();

    printf("  Decode Edge Cases:\n");

    failures += test_decode_null_params();
    failures += test_decode_without_ctx();

    printf("  Controller Role:\n");

    failures += test_controller_send_wrong_role();
    failures += test_controller_send_null_params();

    if (failures == 0)
    {
        printf("OK all context tests passed\n");
        return 0;
    }
    else
    {
        fprintf(stderr, "FAILED %d test(s)\n", failures);
        return 1;
    }
}
