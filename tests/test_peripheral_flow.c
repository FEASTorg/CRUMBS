/*
 * Unit tests for peripheral-side message handling and reply building.
 *
 * Tests the peripheral flow: receiving messages, invoking callbacks,
 * and building reply frames.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"
#include "crumbs_msg.h"

/* ---- Test Infrastructure ---------------------------------------------- */

static int g_on_message_called = 0;
static crumbs_message_t g_last_message;
static void *g_on_message_user_data = NULL;

static int g_on_request_called = 0;
static void *g_on_request_user_data = NULL;

/* Reply message to be returned by on_request */
static crumbs_message_t g_reply_message;
static int g_use_reply = 0;

static void reset_test_state(void)
{
    g_on_message_called = 0;
    memset(&g_last_message, 0, sizeof(g_last_message));
    g_on_message_user_data = NULL;

    g_on_request_called = 0;
    g_on_request_user_data = NULL;

    memset(&g_reply_message, 0, sizeof(g_reply_message));
    g_use_reply = 0;
}

static void test_on_message(crumbs_context_t *ctx, const crumbs_message_t *msg)
{
    g_on_message_called++;
    memcpy(&g_last_message, msg, sizeof(*msg));
    g_on_message_user_data = ctx->user_data;
}

static void test_on_request(crumbs_context_t *ctx, crumbs_message_t *msg)
{
    g_on_request_called++;
    g_on_request_user_data = ctx->user_data;

    if (g_use_reply)
    {
        memcpy(msg, &g_reply_message, sizeof(*msg));
    }
    else
    {
        /* Default minimal reply */
        msg->type_id = 0xFF;
        msg->command_type = 0xFE;
        msg->data_len = 0;
    }
}

/* ---- Peripheral Handle Receive Tests ---------------------------------- */

static int test_handle_receive_valid(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, test_on_message, NULL, (void *)(uintptr_t)0xCAFE);

    reset_test_state();

    /* Build a valid frame */
    crumbs_message_t msg;
    crumbs_msg_init(&msg, 0x42, 0x55);
    crumbs_msg_add_u8(&msg, 0xAA);
    crumbs_msg_add_u16(&msg, 0x1234);

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    int rc = crumbs_peripheral_handle_receive(&ctx, frame, len);
    if (rc != 0)
    {
        fprintf(stderr, "handle_receive_valid: returned %d\n", rc);
        return 1;
    }

    if (g_on_message_called != 1)
    {
        fprintf(stderr, "handle_receive_valid: on_message not called\n");
        return 1;
    }

    if (g_last_message.type_id != 0x42 || g_last_message.command_type != 0x55)
    {
        fprintf(stderr, "handle_receive_valid: header mismatch\n");
        return 1;
    }

    if (g_last_message.data_len != 3)
    {
        fprintf(stderr, "handle_receive_valid: data_len mismatch\n");
        return 1;
    }

    if (g_on_message_user_data != (void *)(uintptr_t)0xCAFE)
    {
        fprintf(stderr, "handle_receive_valid: user_data mismatch\n");
        return 1;
    }

    printf("  handle_receive valid: PASS\n");
    return 0;
}

static int test_handle_receive_wrong_role(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0); /* Wrong role */
    crumbs_set_callbacks(&ctx, test_on_message, NULL, NULL);

    reset_test_state();

    /* Build a valid frame */
    crumbs_message_t msg = {0};
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 0;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    int rc = crumbs_peripheral_handle_receive(&ctx, frame, len);
    if (rc == 0)
    {
        fprintf(stderr, "handle_receive_wrong_role: should fail\n");
        return 1;
    }

    if (g_on_message_called != 0)
    {
        fprintf(stderr, "handle_receive_wrong_role: callback should not be called\n");
        return 1;
    }

    printf("  handle_receive wrong role: PASS\n");
    return 0;
}

static int test_handle_receive_null_params(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);

    uint8_t frame[10] = {0};

    /* NULL ctx */
    int rc = crumbs_peripheral_handle_receive(NULL, frame, sizeof(frame));
    if (rc == 0)
    {
        fprintf(stderr, "handle_receive_null: should fail with NULL ctx\n");
        return 1;
    }

    /* NULL buffer */
    rc = crumbs_peripheral_handle_receive(&ctx, NULL, 10);
    if (rc == 0)
    {
        fprintf(stderr, "handle_receive_null: should fail with NULL buffer\n");
        return 1;
    }

    printf("  handle_receive NULL params: PASS\n");
    return 0;
}

static int test_handle_receive_corrupt_crc(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, test_on_message, NULL, NULL);

    reset_test_state();

    /* Build a valid frame then corrupt it */
    crumbs_message_t msg = {0};
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 2;
    msg.data[0] = 0xAA;
    msg.data[1] = 0xBB;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    /* Corrupt a payload byte */
    frame[3] ^= 0xFF;

    int rc = crumbs_peripheral_handle_receive(&ctx, frame, len);
    if (rc == 0)
    {
        fprintf(stderr, "handle_receive_corrupt: should fail\n");
        return 1;
    }

    if (g_on_message_called != 0)
    {
        fprintf(stderr, "handle_receive_corrupt: callback should not be called\n");
        return 1;
    }

    /* CRC error count should increment */
    if (crumbs_get_crc_error_count(&ctx) != 1)
    {
        fprintf(stderr, "handle_receive_corrupt: crc_error_count should be 1\n");
        return 1;
    }

    printf("  handle_receive corrupt CRC: PASS\n");
    return 0;
}

static int test_handle_receive_no_callback(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    /* No callbacks set */

    /* Build a valid frame */
    crumbs_message_t msg = {0};
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 0;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    /* Should still succeed (just no callback invoked) */
    int rc = crumbs_peripheral_handle_receive(&ctx, frame, len);
    if (rc != 0)
    {
        fprintf(stderr, "handle_receive_no_callback: should succeed\n");
        return 1;
    }

    printf("  handle_receive no callback: PASS\n");
    return 0;
}

static int test_handle_receive_sets_address(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x42);
    crumbs_set_callbacks(&ctx, test_on_message, NULL, NULL);

    reset_test_state();

    /* Build a valid frame */
    crumbs_message_t msg = {0};
    msg.type_id = 0x01;
    msg.command_type = 0x02;
    msg.data_len = 0;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));

    crumbs_peripheral_handle_receive(&ctx, frame, len);

    /* The message passed to callback should have address set from ctx */
    if (g_last_message.address != 0x42)
    {
        fprintf(stderr, "handle_receive_sets_address: address should be 0x42, got 0x%02X\n",
                g_last_message.address);
        return 1;
    }

    printf("  handle_receive sets address: PASS\n");
    return 0;
}

/* ---- Peripheral Build Reply Tests ------------------------------------- */

static int test_build_reply_valid(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, (void *)(uintptr_t)0xBEEF);

    reset_test_state();

    g_use_reply = 1;
    crumbs_msg_init(&g_reply_message, 0x42, 0x55);
    crumbs_msg_add_u8(&g_reply_message, 0xAA);

    uint8_t out_buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t out_len = 0;

    int rc = crumbs_peripheral_build_reply(&ctx, out_buf, sizeof(out_buf), &out_len);
    if (rc != 0)
    {
        fprintf(stderr, "build_reply_valid: returned %d\n", rc);
        return 1;
    }

    if (g_on_request_called != 1)
    {
        fprintf(stderr, "build_reply_valid: on_request not called\n");
        return 1;
    }

    if (g_on_request_user_data != (void *)(uintptr_t)0xBEEF)
    {
        fprintf(stderr, "build_reply_valid: user_data mismatch\n");
        return 1;
    }

    /* Verify the encoded frame */
    if (out_len != 4 + 1) /* header + 1 byte payload + CRC */
    {
        fprintf(stderr, "build_reply_valid: out_len should be 5, got %zu\n", out_len);
        return 1;
    }

    /* Decode and verify */
    crumbs_message_t decoded;
    rc = crumbs_decode_message(out_buf, out_len, &decoded, NULL);
    if (rc != 0)
    {
        fprintf(stderr, "build_reply_valid: decode failed\n");
        return 1;
    }

    if (decoded.type_id != 0x42 || decoded.command_type != 0x55)
    {
        fprintf(stderr, "build_reply_valid: decoded header mismatch\n");
        return 1;
    }

    printf("  build_reply valid: PASS\n");
    return 0;
}

static int test_build_reply_no_callback(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    /* No on_request callback */

    uint8_t out_buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t out_len = 99; /* Should be set to 0 */

    int rc = crumbs_peripheral_build_reply(&ctx, out_buf, sizeof(out_buf), &out_len);
    if (rc != 0)
    {
        fprintf(stderr, "build_reply_no_callback: should succeed\n");
        return 1;
    }

    if (out_len != 0)
    {
        fprintf(stderr, "build_reply_no_callback: out_len should be 0\n");
        return 1;
    }

    printf("  build_reply no callback: PASS\n");
    return 0;
}

static int test_build_reply_wrong_role(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0); /* Wrong role */
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    reset_test_state();

    uint8_t out_buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t out_len = 0;

    int rc = crumbs_peripheral_build_reply(&ctx, out_buf, sizeof(out_buf), &out_len);
    if (rc == 0)
    {
        fprintf(stderr, "build_reply_wrong_role: should fail\n");
        return 1;
    }

    if (g_on_request_called != 0)
    {
        fprintf(stderr, "build_reply_wrong_role: callback should not be called\n");
        return 1;
    }

    printf("  build_reply wrong role: PASS\n");
    return 0;
}

static int test_build_reply_null_params(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    uint8_t out_buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t out_len = 0;

    /* NULL ctx */
    int rc = crumbs_peripheral_build_reply(NULL, out_buf, sizeof(out_buf), &out_len);
    if (rc == 0)
    {
        fprintf(stderr, "build_reply_null: should fail with NULL ctx\n");
        return 1;
    }

    /* NULL out_buf */
    rc = crumbs_peripheral_build_reply(&ctx, NULL, sizeof(out_buf), &out_len);
    if (rc == 0)
    {
        fprintf(stderr, "build_reply_null: should fail with NULL out_buf\n");
        return 1;
    }

    /* NULL out_len is allowed - the implementation handles it gracefully */
    rc = crumbs_peripheral_build_reply(&ctx, out_buf, sizeof(out_buf), NULL);
    if (rc != 0)
    {
        fprintf(stderr, "build_reply_null: NULL out_len should be allowed\n");
        return 1;
    }

    printf("  build_reply NULL params: PASS\n");
    return 0;
}

static int test_build_reply_buffer_too_small(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    reset_test_state();

    g_use_reply = 1;
    crumbs_msg_init(&g_reply_message, 0x42, 0x55);
    /* Add 10 bytes of data */
    for (int i = 0; i < 10; ++i)
        crumbs_msg_add_u8(&g_reply_message, (uint8_t)i);

    /* Buffer too small (need 4 + 10 = 14 bytes) */
    uint8_t out_buf[10];
    size_t out_len = 0;

    int rc = crumbs_peripheral_build_reply(&ctx, out_buf, sizeof(out_buf), &out_len);
    if (rc == 0)
    {
        fprintf(stderr, "build_reply_buffer_too_small: should fail\n");
        return 1;
    }

    printf("  build_reply buffer too small: PASS\n");
    return 0;
}

static int test_build_reply_max_payload(void)
{
    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    reset_test_state();

    g_use_reply = 1;
    crumbs_msg_init(&g_reply_message, 0x42, 0x55);
    /* Fill with max payload */
    for (int i = 0; i < CRUMBS_MAX_PAYLOAD; ++i)
        crumbs_msg_add_u8(&g_reply_message, (uint8_t)i);

    uint8_t out_buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t out_len = 0;

    int rc = crumbs_peripheral_build_reply(&ctx, out_buf, sizeof(out_buf), &out_len);
    if (rc != 0)
    {
        fprintf(stderr, "build_reply_max_payload: returned %d\n", rc);
        return 1;
    }

    if (out_len != CRUMBS_MESSAGE_MAX_SIZE)
    {
        fprintf(stderr, "build_reply_max_payload: out_len should be %u, got %zu\n",
                CRUMBS_MESSAGE_MAX_SIZE, out_len);
        return 1;
    }

    /* Verify the frame is valid */
    crumbs_message_t decoded;
    rc = crumbs_decode_message(out_buf, out_len, &decoded, NULL);
    if (rc != 0)
    {
        fprintf(stderr, "build_reply_max_payload: decode failed\n");
        return 1;
    }

    if (decoded.data_len != CRUMBS_MAX_PAYLOAD)
    {
        fprintf(stderr, "build_reply_max_payload: data_len mismatch\n");
        return 1;
    }

    printf("  build_reply max payload: PASS\n");
    return 0;
}

/* ---- Main ------------------------------------------------------------- */

int main(void)
{
    int failures = 0;

    printf("Running peripheral flow tests:\n");
    printf("  Handle Receive:\n");

    failures += test_handle_receive_valid();
    failures += test_handle_receive_wrong_role();
    failures += test_handle_receive_null_params();
    failures += test_handle_receive_corrupt_crc();
    failures += test_handle_receive_no_callback();
    failures += test_handle_receive_sets_address();

    printf("  Build Reply:\n");

    failures += test_build_reply_valid();
    failures += test_build_reply_no_callback();
    failures += test_build_reply_wrong_role();
    failures += test_build_reply_null_params();
    failures += test_build_reply_buffer_too_small();
    failures += test_build_reply_max_payload();

    if (failures == 0)
    {
        printf("OK all peripheral flow tests passed\n");
        return 0;
    }
    else
    {
        fprintf(stderr, "FAILED %d test(s)\n", failures);
        return 1;
    }
}
