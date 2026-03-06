/*
 * Integration tests for the SET_REPLY flow.
 *
 * Tests the end-to-end behavior of SET_REPLY mechanism:
 * - Controller sends SET_REPLY, peripheral stores target opcode
 * - Peripheral's on_request callback switches on requested_opcode
 * - Reply is built correctly with proper CRC
 *
 * This simulates the complete query flow without actual I2C hardware.
 */

#include "test_common.h"
#include "crumbs_message_helpers.h"

#include <string.h> /* memcpy */

/* ---- Test Infrastructure ---------------------------------------------- */

#define MY_TYPE_ID 0x42
#define MODULE_VER_MAJ 1
#define MODULE_VER_MIN 2
#define MODULE_VER_PAT 3

/* Simulated peripheral state */
static uint16_t g_sensor_value = 0x1234;
static uint8_t g_status_byte = 0xAB;

/**
 * @brief Peripheral's on_request callback that switches on requested_opcode.
 */
static void test_on_request(crumbs_context_t *ctx, crumbs_message_t *reply)
{
    switch (ctx->requested_opcode)
    {
    case 0x00: /* Default: device info */
        crumbs_msg_init(reply, MY_TYPE_ID, 0x00);
        crumbs_msg_add_u16(reply, CRUMBS_VERSION);
        crumbs_msg_add_u8(reply, MODULE_VER_MAJ);
        crumbs_msg_add_u8(reply, MODULE_VER_MIN);
        crumbs_msg_add_u8(reply, MODULE_VER_PAT);
        break;

    case 0x10: /* Sensor value */
        crumbs_msg_init(reply, MY_TYPE_ID, 0x10);
        crumbs_msg_add_u16(reply, g_sensor_value);
        break;

    case 0x11: /* Status byte */
        crumbs_msg_init(reply, MY_TYPE_ID, 0x11);
        crumbs_msg_add_u8(reply, g_status_byte);
        break;

    default: /* Unknown opcode - return empty */
        crumbs_msg_init(reply, MY_TYPE_ID, ctx->requested_opcode);
        break;
    }
}

/**
 * @brief Helper to simulate controller sending SET_REPLY to peripheral.
 */
static int simulate_set_reply(crumbs_context_t *peripheral_ctx, uint8_t target_opcode)
{
    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    crumbs_message_t msg;
    crumbs_msg_init(&msg, MY_TYPE_ID, CRUMBS_CMD_SET_REPLY);
    crumbs_msg_add_u8(&msg, target_opcode);

    size_t len = crumbs_encode_message(&msg, frame, sizeof(frame));
    if (len == 0)
        return -1;

    return crumbs_peripheral_handle_receive(peripheral_ctx, frame, len);
}

/**
 * @brief Helper to simulate controller reading reply from peripheral.
 */
static int simulate_read_reply(crumbs_context_t *peripheral_ctx,
                               crumbs_message_t *decoded_reply)
{
    uint8_t reply_frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t reply_len = 0;

    int rc = crumbs_peripheral_build_reply(peripheral_ctx, reply_frame, sizeof(reply_frame), &reply_len);
    if (rc != 0)
        return rc;
    if (reply_len == 0)
        return -1;

    /* Decode the reply to verify CRC and contents */
    return crumbs_decode_message(reply_frame, reply_len, decoded_reply, NULL);
}

/* ---- Mock I2C read for crumbs_controller_read tests ------------------ */

/** Buffer staged by tests for the mock read function to return. */
static uint8_t g_mock_read_buf[CRUMBS_MESSAGE_MAX_SIZE];
/** Length to return from the mock read function. Negative = simulate error. */
static int g_mock_read_len = 0;

/**
 * @brief Mock crumbs_i2c_read_fn: returns bytes from g_mock_read_buf.
 */
static int mock_i2c_read(void *user_ctx,
                         uint8_t addr,
                         uint8_t *buffer,
                         size_t len,
                         uint32_t timeout_us)
{
    (void)user_ctx;
    (void)addr;
    (void)timeout_us;

    if (g_mock_read_len < 0)
        return g_mock_read_len; /* simulated I2C error */

    size_t n = (size_t)g_mock_read_len < len ? (size_t)g_mock_read_len : len;
    memcpy(buffer, g_mock_read_buf, n);
    return g_mock_read_len;
}

/* ---- Tests ------------------------------------------------------------ */

/**
 * Test: Default reply (opcode 0x00) returns device info.
 */
static int test_default_reply(void)
{
    const char *test_name = "default_reply";

    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    /* Without any SET_REPLY, requested_opcode should be 0 */
    TEST_ASSERT_EQ(test_name, ctx.requested_opcode, 0x00,
                   "initial requested_opcode should be 0");

    crumbs_message_t reply;
    int rc = simulate_read_reply(&ctx, &reply);
    TEST_ASSERT_EQ(test_name, rc, 0, "read reply should succeed");

    /* Verify device info payload */
    TEST_ASSERT_EQ(test_name, reply.type_id, MY_TYPE_ID, "type_id mismatch");
    TEST_ASSERT_EQ(test_name, reply.opcode, 0x00, "opcode should be 0x00");
    TEST_ASSERT_EQ(test_name, reply.data_len, 5, "should have 5 bytes payload");

    /* Decode version info: u16 + u8 + u8 + u8 */
    uint16_t lib_ver = (uint16_t)(reply.data[0] | (reply.data[1] << 8));
    TEST_ASSERT_EQ(test_name, lib_ver, CRUMBS_VERSION, "CRUMBS_VERSION mismatch");
    TEST_ASSERT_EQ(test_name, reply.data[2], MODULE_VER_MAJ, "major version mismatch");
    TEST_ASSERT_EQ(test_name, reply.data[3], MODULE_VER_MIN, "minor version mismatch");
    TEST_ASSERT_EQ(test_name, reply.data[4], MODULE_VER_PAT, "patch version mismatch");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/**
 * Test: SET_REPLY then read returns correct data.
 */
static int test_simple_query_flow(void)
{
    const char *test_name = "simple_query_flow";

    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    /* Controller sends SET_REPLY(0x10) to request sensor value */
    int rc = simulate_set_reply(&ctx, 0x10);
    TEST_ASSERT_EQ(test_name, rc, 0, "SET_REPLY should succeed");
    TEST_ASSERT_EQ(test_name, ctx.requested_opcode, 0x10, "requested_opcode should be 0x10");

    /* Controller reads reply */
    crumbs_message_t reply;
    rc = simulate_read_reply(&ctx, &reply);
    TEST_ASSERT_EQ(test_name, rc, 0, "read reply should succeed");

    /* Verify sensor value payload */
    TEST_ASSERT_EQ(test_name, reply.opcode, 0x10, "opcode should be 0x10");
    TEST_ASSERT_EQ(test_name, reply.data_len, 2, "should have 2 bytes (u16)");

    uint16_t sensor = (uint16_t)(reply.data[0] | (reply.data[1] << 8));
    TEST_ASSERT_EQ(test_name, sensor, g_sensor_value, "sensor value mismatch");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/**
 * Test: Multiple SET_REPLY changes what data is returned.
 */
static int test_multi_opcode_flow(void)
{
    const char *test_name = "multi_opcode_flow";

    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    crumbs_message_t reply;
    int rc;

    /* First: request sensor value (0x10) */
    rc = simulate_set_reply(&ctx, 0x10);
    TEST_ASSERT_EQ(test_name, rc, 0, "SET_REPLY 0x10 should succeed");

    rc = simulate_read_reply(&ctx, &reply);
    TEST_ASSERT_EQ(test_name, rc, 0, "read 0x10 should succeed");
    TEST_ASSERT_EQ(test_name, reply.opcode, 0x10, "first reply should be 0x10");
    TEST_ASSERT_EQ(test_name, reply.data_len, 2, "sensor is 2 bytes");

    /* Second: request status byte (0x11) */
    rc = simulate_set_reply(&ctx, 0x11);
    TEST_ASSERT_EQ(test_name, rc, 0, "SET_REPLY 0x11 should succeed");

    rc = simulate_read_reply(&ctx, &reply);
    TEST_ASSERT_EQ(test_name, rc, 0, "read 0x11 should succeed");
    TEST_ASSERT_EQ(test_name, reply.opcode, 0x11, "second reply should be 0x11");
    TEST_ASSERT_EQ(test_name, reply.data_len, 1, "status is 1 byte");
    TEST_ASSERT_EQ(test_name, reply.data[0], g_status_byte, "status byte mismatch");

    /* Third: back to device info (0x00) */
    rc = simulate_set_reply(&ctx, 0x00);
    TEST_ASSERT_EQ(test_name, rc, 0, "SET_REPLY 0x00 should succeed");

    rc = simulate_read_reply(&ctx, &reply);
    TEST_ASSERT_EQ(test_name, rc, 0, "read 0x00 should succeed");
    TEST_ASSERT_EQ(test_name, reply.opcode, 0x00, "third reply should be 0x00");
    TEST_ASSERT_EQ(test_name, reply.data_len, 5, "device info is 5 bytes");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/**
 * Test: Unknown opcode returns empty payload with that opcode.
 */
static int test_unknown_opcode(void)
{
    const char *test_name = "unknown_opcode";

    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    /* Request unknown opcode 0xFF */
    int rc = simulate_set_reply(&ctx, 0xFF);
    TEST_ASSERT_EQ(test_name, rc, 0, "SET_REPLY 0xFF should succeed");

    crumbs_message_t reply;
    rc = simulate_read_reply(&ctx, &reply);
    TEST_ASSERT_EQ(test_name, rc, 0, "read should succeed");

    /* Handler returns empty message with the requested opcode */
    TEST_ASSERT_EQ(test_name, reply.opcode, 0xFF, "opcode should echo 0xFF");
    TEST_ASSERT_EQ(test_name, reply.data_len, 0, "unknown opcode returns empty payload");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/**
 * Test: Reply CRC is valid.
 */
static int test_reply_crc(void)
{
    const char *test_name = "reply_crc";

    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    simulate_set_reply(&ctx, 0x10);

    /* Build raw reply frame */
    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t frame_len = 0;
    int rc = crumbs_peripheral_build_reply(&ctx, frame, sizeof(frame), &frame_len);
    TEST_ASSERT_EQ(test_name, rc, 0, "build_reply should succeed");
    TEST_ASSERT(test_name, frame_len >= 4, "frame should be at least 4 bytes");

    /* Decode should succeed if CRC is valid */
    crumbs_message_t decoded;
    rc = crumbs_decode_message(frame, frame_len, &decoded, NULL);
    TEST_ASSERT_EQ(test_name, rc, 0, "decode should succeed (CRC valid)");

    /* Corrupt one byte and verify CRC fails */
    frame[1] ^= 0x01; /* flip a bit in opcode */
    rc = crumbs_decode_message(frame, frame_len, &decoded, NULL);
    TEST_ASSERT_EQ(test_name, rc, -2, "corrupted frame should fail CRC");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/**
 * Test: Repeated reads return same data (requested_opcode persists).
 */
static int test_persistent_opcode(void)
{
    const char *test_name = "persistent_opcode";

    crumbs_context_t ctx = {0};
    crumbs_init(&ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
    crumbs_set_callbacks(&ctx, NULL, test_on_request, NULL);

    simulate_set_reply(&ctx, 0x10);

    crumbs_message_t reply1, reply2;

    /* First read */
    int rc = simulate_read_reply(&ctx, &reply1);
    TEST_ASSERT_EQ(test_name, rc, 0, "first read should succeed");

    /* Second read without new SET_REPLY */
    rc = simulate_read_reply(&ctx, &reply2);
    TEST_ASSERT_EQ(test_name, rc, 0, "second read should succeed");

    /* Both should return same opcode */
    TEST_ASSERT_EQ(test_name, reply1.opcode, reply2.opcode, "opcodes should match");
    TEST_ASSERT_EQ(test_name, reply1.data_len, reply2.data_len, "data_len should match");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/**
 * Test: crumbs_controller_read successfully decodes a valid reply frame.
 */
static int test_controller_read_ok(void)
{
    const char *test_name = "controller_read_ok";

    /* Set up a peripheral and build a valid reply frame into the mock buffer. */
    crumbs_context_t periph = {0};
    crumbs_init(&periph, CRUMBS_ROLE_PERIPHERAL, 0x20);
    crumbs_set_callbacks(&periph, NULL, test_on_request, NULL);

    /* Prime peripheral for opcode 0x10 (sensor value). */
    int rc = simulate_set_reply(&periph, 0x10);
    TEST_ASSERT_EQ(test_name, rc, 0, "simulate_set_reply should succeed");

    /* Build the encoded reply frame into the mock buffer. */
    size_t frame_len = 0;
    rc = crumbs_peripheral_build_reply(&periph, g_mock_read_buf, sizeof(g_mock_read_buf), &frame_len);
    TEST_ASSERT_EQ(test_name, rc, 0, "build_reply should succeed");
    TEST_ASSERT(test_name, frame_len >= 4, "frame_len should be >= 4");
    g_mock_read_len = (int)frame_len;

    /* Now test crumbs_controller_read on the controller side. */
    crumbs_context_t ctrl = {0};
    crumbs_init(&ctrl, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t out_msg;
    rc = crumbs_controller_read(&ctrl, 0x20, &out_msg, mock_i2c_read, NULL);
    TEST_ASSERT_EQ(test_name, rc, 0, "crumbs_controller_read should succeed");
    TEST_ASSERT_EQ(test_name, out_msg.opcode, 0x10, "opcode should be 0x10");
    TEST_ASSERT_EQ(test_name, out_msg.data_len, 2, "data_len should be 2 (u16)");

    uint16_t sensor = (uint16_t)(out_msg.data[0] | (out_msg.data[1] << 8));
    TEST_ASSERT_EQ(test_name, sensor, g_sensor_value, "sensor value mismatch");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/**
 * Test: crumbs_controller_read returns -1 when read_fn supplies a short frame.
 */
static int test_controller_read_short(void)
{
    const char *test_name = "controller_read_short";

    crumbs_context_t ctrl = {0};
    crumbs_init(&ctrl, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t out_msg;

    /* 3 bytes: one fewer than the minimum frame size (4). */
    g_mock_read_buf[0] = 0x42;
    g_mock_read_buf[1] = 0x10;
    g_mock_read_buf[2] = 0x00;
    g_mock_read_len = 3;

    int rc = crumbs_controller_read(&ctrl, 0x20, &out_msg, mock_i2c_read, NULL);
    TEST_ASSERT_EQ(test_name, rc, -1, "should return -1 on short read");

    /* 0 bytes: empty read. */
    g_mock_read_len = 0;
    rc = crumbs_controller_read(&ctrl, 0x20, &out_msg, mock_i2c_read, NULL);
    TEST_ASSERT_EQ(test_name, rc, -1, "should return -1 on zero-byte read");

    printf("  %s: PASS\n", test_name);
    return 0;
}

/* ---- Main ------------------------------------------------------------- */

int main(void)
{
    int failures = 0;

    printf("=== test_reply_flow ===\n");

    failures += test_default_reply();
    failures += test_simple_query_flow();
    failures += test_multi_opcode_flow();
    failures += test_unknown_opcode();
    failures += test_reply_crc();
    failures += test_persistent_opcode();
    failures += test_controller_read_ok();
    failures += test_controller_read_short();

    printf("\n");
    if (failures == 0)
    {
        printf("All reply flow tests passed!\n");
    }
    else
    {
        printf("FAILED: %d test(s)\n", failures);
    }

    return failures;
}
