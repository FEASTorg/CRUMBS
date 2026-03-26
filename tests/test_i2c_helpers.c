/*
 * Tests for raw I2C helper APIs and candidate-list scanning.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "test_common.h"

typedef struct
{
    uint8_t last_addr;
    uint8_t last_write[128];
    size_t last_write_len;
    int write_calls;
    int write_fail;

    uint8_t read_src[128];
    size_t read_src_len;
    int read_calls;
    int read_fail;
    int force_short_read;

    uint8_t combined_last_tx[128];
    size_t combined_last_tx_len;
    int combined_last_require_rs;
    uint8_t combined_rx[128];
    size_t combined_rx_len;
    int combined_calls;
    int combined_fail;
    int combined_no_rs;
} helper_io_t;

static void helper_io_reset(helper_io_t *io)
{
    memset(io, 0, sizeof(*io));
    io->force_short_read = -1;
}

static int helper_write(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len)
{
    helper_io_t *io = (helper_io_t *)user_ctx;
    if (!io || (len > 0u && !data))
        return -1;
    if (io->write_fail)
        return -1;

    io->write_calls++;
    io->last_addr = addr;
    io->last_write_len = len;
    if (len > 0u && len <= sizeof(io->last_write))
        memcpy(io->last_write, data, len);
    return 0;
}

static int helper_read(void *user_ctx, uint8_t addr, uint8_t *buffer, size_t len, uint32_t timeout_us)
{
    (void)timeout_us;
    helper_io_t *io = (helper_io_t *)user_ctx;
    if (!io || !buffer || len == 0u)
        return -1;
    if (io->read_fail)
        return -1;

    io->read_calls++;
    io->last_addr = addr;

    size_t n = io->read_src_len;
    if (n > len)
        n = len;
    if (io->force_short_read >= 0 && (size_t)io->force_short_read < n)
        n = (size_t)io->force_short_read;

    if (n > 0u)
        memcpy(buffer, io->read_src, n);
    return (int)n;
}

static int helper_write_read(void *user_ctx,
                             uint8_t addr,
                             const uint8_t *tx,
                             size_t tx_len,
                             uint8_t *rx,
                             size_t rx_len,
                             uint32_t timeout_us,
                             int require_repeated_start)
{
    (void)timeout_us;
    helper_io_t *io = (helper_io_t *)user_ctx;
    if (!io || (tx_len > 0u && !tx) || (rx_len > 0u && !rx))
        return -1;

    io->combined_calls++;
    io->last_addr = addr;
    io->combined_last_tx_len = tx_len;
    io->combined_last_require_rs = require_repeated_start;
    if (tx_len > 0u && tx_len <= sizeof(io->combined_last_tx))
        memcpy(io->combined_last_tx, tx, tx_len);

    if (io->combined_no_rs && require_repeated_start)
        return CRUMBS_I2C_DEV_E_NO_REPEATED_START;
    if (io->combined_fail)
        return -1;

    size_t n = io->combined_rx_len;
    if (n > rx_len)
        n = rx_len;
    if (n > 0u)
        memcpy(rx, io->combined_rx, n);
    return (int)n;
}

static int test_dev_write_and_read(void)
{
    const char *t = "dev_write_and_read";
    helper_io_t io;
    helper_io_reset(&io);
    io.read_src[0] = 0x10;
    io.read_src[1] = 0x11;
    io.read_src[2] = 0x12;
    io.read_src_len = 3;

    crumbs_context_t ctx;
    test_init_controller(&ctx);
    crumbs_device_t dev = {&ctx, 0x20, helper_write, helper_read, NULL, &io};

    uint8_t tx[2] = {0xAA, 0xBB};
    int rc = crumbs_i2c_dev_write(&dev, tx, sizeof(tx));
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "write failed");
    TEST_ASSERT_EQ(t, io.write_calls, 1, "write callback not called");
    TEST_ASSERT_EQ(t, io.last_addr, 0x20, "wrong write addr");
    TEST_ASSERT_SIZE_EQ(t, io.last_write_len, sizeof(tx), "wrong write len");

    uint8_t rx[3] = {0};
    rc = crumbs_i2c_dev_read(&dev, rx, sizeof(rx), 1000);
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "read failed");
    TEST_ASSERT_EQ(t, io.read_calls, 1, "read callback not called");
    TEST_ASSERT(t, memcmp(rx, io.read_src, sizeof(rx)) == 0, "read data mismatch");
    return 0;
}

static int test_write_then_read_fallback_and_repeated_requirements(void)
{
    const char *t = "write_then_read_fallback_and_repeated_requirements";
    helper_io_t io;
    helper_io_reset(&io);
    io.read_src[0] = 0x01;
    io.read_src[1] = 0x02;
    io.read_src_len = 2;

    crumbs_context_t ctx;
    test_init_controller(&ctx);
    crumbs_device_t dev = {&ctx, 0x21, helper_write, helper_read, NULL, &io};

    uint8_t tx[1] = {0xA0};
    uint8_t rx[2] = {0};

    int rc = crumbs_i2c_dev_write_then_read(&dev, tx, sizeof(tx), rx, sizeof(rx),
                                            5000, 0, NULL);
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "fallback write+read failed");
    TEST_ASSERT_EQ(t, io.write_calls, 1, "fallback write not used");
    TEST_ASSERT_EQ(t, io.read_calls, 1, "fallback read not used");

    rc = crumbs_i2c_dev_write_then_read(&dev, tx, sizeof(tx), rx, sizeof(rx),
                                        5000, 1, NULL);
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_E_NO_REPEATED_START,
                   "missing repeated-start callback not detected");
    return 0;
}

static int test_write_then_read_combined(void)
{
    const char *t = "write_then_read_combined";
    helper_io_t io;
    helper_io_reset(&io);
    io.combined_rx[0] = 0x33;
    io.combined_rx[1] = 0x44;
    io.combined_rx_len = 2;

    crumbs_context_t ctx;
    test_init_controller(&ctx);
    crumbs_device_t dev = {&ctx, 0x22, helper_write, helper_read, NULL, &io};

    uint8_t tx[2] = {0x0A, 0x0B};
    uint8_t rx[2] = {0};
    int rc = crumbs_i2c_dev_write_then_read(&dev, tx, sizeof(tx), rx, sizeof(rx),
                                            5000, 1, helper_write_read);
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "combined callback path failed");
    TEST_ASSERT_EQ(t, io.combined_calls, 1, "combined callback not used");
    TEST_ASSERT_EQ(t, io.write_calls, 0, "fallback write should not run");
    TEST_ASSERT_EQ(t, io.read_calls, 0, "fallback read should not run");
    TEST_ASSERT_EQ(t, io.combined_last_require_rs, 1, "require_repeated_start not forwarded");
    TEST_ASSERT(t, memcmp(rx, io.combined_rx, sizeof(rx)) == 0, "combined rx mismatch");

    io.combined_no_rs = 1;
    rc = crumbs_i2c_dev_write_then_read(&dev, tx, sizeof(tx), rx, sizeof(rx),
                                        5000, 1, helper_write_read);
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_E_NO_REPEATED_START,
                   "combined no-repeated-start path not surfaced");
    return 0;
}

static int test_reg_helpers(void)
{
    const char *t = "reg_helpers";
    helper_io_t io;
    helper_io_reset(&io);
    io.combined_rx[0] = 0xFE;
    io.combined_rx[1] = 0xED;
    io.combined_rx_len = 2;

    crumbs_context_t ctx;
    test_init_controller(&ctx);
    crumbs_device_t dev = {&ctx, 0x23, helper_write, helper_read, NULL, &io};

    uint8_t out[2] = {0};
    int rc = crumbs_i2c_dev_read_reg_u8(&dev, 0x12, out, sizeof(out),
                                        7000, 1, helper_write_read);
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "read_reg_u8 failed");
    TEST_ASSERT_SIZE_EQ(t, io.combined_last_tx_len, 1u, "u8 reg tx len");
    TEST_ASSERT_EQ(t, io.combined_last_tx[0], 0x12, "u8 reg value");

    rc = crumbs_i2c_dev_read_reg_u16be(&dev, 0x1234, out, sizeof(out),
                                       7000, 1, helper_write_read);
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "read_reg_u16be failed");
    TEST_ASSERT_SIZE_EQ(t, io.combined_last_tx_len, 2u, "u16 reg tx len");
    TEST_ASSERT_EQ(t, io.combined_last_tx[0], 0x12, "u16 reg high byte");
    TEST_ASSERT_EQ(t, io.combined_last_tx[1], 0x34, "u16 reg low byte");

    uint8_t w1[2] = {0x9A, 0xBC};
    rc = crumbs_i2c_dev_write_reg_u8(&dev, 0x21, w1, sizeof(w1));
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "write_reg_u8 failed");
    TEST_ASSERT_SIZE_EQ(t, io.last_write_len, 3u, "write_reg_u8 len");
    TEST_ASSERT_EQ(t, io.last_write[0], 0x21, "write_reg_u8 reg");
    TEST_ASSERT_EQ(t, io.last_write[1], 0x9A, "write_reg_u8 data[0]");
    TEST_ASSERT_EQ(t, io.last_write[2], 0xBC, "write_reg_u8 data[1]");

    uint8_t w2[1] = {0x55};
    rc = crumbs_i2c_dev_write_reg_u16be(&dev, 0xABCD, w2, sizeof(w2));
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_OK, "write_reg_u16be failed");
    TEST_ASSERT_SIZE_EQ(t, io.last_write_len, 3u, "write_reg_u16be len");
    TEST_ASSERT_EQ(t, io.last_write[0], 0xAB, "write_reg_u16be reg high");
    TEST_ASSERT_EQ(t, io.last_write[1], 0xCD, "write_reg_u16be reg low");
    TEST_ASSERT_EQ(t, io.last_write[2], 0x55, "write_reg_u16be data");

    uint8_t reg_buf[40];
    uint8_t data_buf[40];
    memset(reg_buf, 0x11, sizeof(reg_buf));
    memset(data_buf, 0x22, sizeof(data_buf));
    rc = crumbs_i2c_dev_write_reg_ex(&dev, reg_buf, sizeof(reg_buf), data_buf, sizeof(data_buf));
    TEST_ASSERT_EQ(t, rc, CRUMBS_I2C_DEV_E_SIZE, "oversized register write not rejected");
    return 0;
}

typedef struct
{
    uint8_t has_immediate[128];
    uint8_t needs_probe[128];
    uint8_t probe_seen[128];
    uint8_t read_seen[128];
    uint8_t read_count[128];
    uint8_t write_seen[128];
} scan_io_t;

static void scan_io_reset(scan_io_t *io)
{
    memset(io, 0, sizeof(*io));
}

static int scan_read_build_frame(uint8_t addr, uint8_t *buffer, size_t len)
{
    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = addr;
    m.opcode = 0x01;
    m.data_len = 1;
    m.data[0] = 0xAA;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));
    if (w == 0u || w > len)
        return 0;
    memcpy(buffer, frame, w);
    return (int)w;
}

static int scan_write(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len)
{
    (void)data;
    (void)len;
    scan_io_t *io = (scan_io_t *)user_ctx;
    if (!io)
        return -1;
    io->write_seen[addr] = 1u;
    io->probe_seen[addr] = 1u;
    return 0;
}

static int scan_read(void *user_ctx, uint8_t addr, uint8_t *buffer, size_t len, uint32_t timeout_us)
{
    (void)timeout_us;
    scan_io_t *io = (scan_io_t *)user_ctx;
    if (!io || !buffer || len == 0u)
        return -1;

    io->read_seen[addr] = 1u;
    io->read_count[addr]++;

    if (io->has_immediate[addr] || (io->needs_probe[addr] && io->probe_seen[addr]))
        return scan_read_build_frame(addr, buffer, len);

    return 0;
}

static int addr_in_set(uint8_t addr, const uint8_t *set, size_t set_len)
{
    for (size_t i = 0; i < set_len; ++i)
    {
        if (set[i] == addr)
            return 1;
    }
    return 0;
}

static int test_scan_candidates_strict(void)
{
    const char *t = "scan_candidates_strict";
    scan_io_t io;
    scan_io_reset(&io);
    io.has_immediate[0x10] = 1u;
    io.has_immediate[0x20] = 1u;

    crumbs_context_t ctx;
    test_init_controller(&ctx);

    uint8_t candidates[] = {0x10, 0x20, 0x55};
    uint8_t found[4] = {0};
    uint8_t types[4] = {0};
    int n = crumbs_controller_scan_for_crumbs_candidates(
        &ctx, candidates, sizeof(candidates), 1,
        NULL, scan_read, &io, found, types, 4, 10000);
    TEST_ASSERT_EQ(t, n, 2, "strict candidate scan should find two devices");

    for (int a = 0; a < 128; ++a)
    {
        if (io.read_seen[a] && !addr_in_set((uint8_t)a, candidates, sizeof(candidates)))
        {
            TEST_ASSERT(t, 0, "read hit non-candidate address");
        }
    }
    return 0;
}

static int test_scan_candidates_non_strict_probe(void)
{
    const char *t = "scan_candidates_non_strict_probe";
    scan_io_t io;
    scan_io_reset(&io);
    io.has_immediate[0x10] = 1u;
    io.needs_probe[0x30] = 1u;

    crumbs_context_t ctx;
    test_init_controller(&ctx);

    uint8_t candidates[] = {0x10, 0x30, 0x44};
    uint8_t found[4] = {0};
    int n = crumbs_controller_scan_for_crumbs_candidates(
        &ctx, candidates, sizeof(candidates), 0,
        scan_write, scan_read, &io, found, NULL, 4, 10000);
    TEST_ASSERT_EQ(t, n, 2, "non-strict candidate scan should find probe + immediate devices");
    TEST_ASSERT_EQ(t, io.probe_seen[0x30], 1, "probe-required address not probed");

    for (int a = 0; a < 128; ++a)
    {
        if (io.write_seen[a] && !addr_in_set((uint8_t)a, candidates, sizeof(candidates)))
        {
            TEST_ASSERT(t, 0, "probe write hit non-candidate address");
        }
    }
    return 0;
}

static int test_scan_candidates_dedupes(void)
{
    const char *t = "scan_candidates_dedupes";
    scan_io_t io;
    scan_io_reset(&io);
    io.has_immediate[0x10] = 1u;

    crumbs_context_t ctx;
    test_init_controller(&ctx);

    uint8_t candidates[] = {0x10, 0x10, 0x10};
    uint8_t found[4] = {0};
    int n = crumbs_controller_scan_for_crumbs_candidates(
        &ctx, candidates, sizeof(candidates), 1,
        NULL, scan_read, &io, found, NULL, 4, 10000);
    TEST_ASSERT_EQ(t, n, 1, "dedupe should return one device");
    TEST_ASSERT_EQ(t, io.read_count[0x10], 1, "duplicate candidate was probed multiple times");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("Running i2c helper tests:\n");
    failures += test_dev_write_and_read();
    failures += test_write_then_read_fallback_and_repeated_requirements();
    failures += test_write_then_read_combined();
    failures += test_reg_helpers();
    failures += test_scan_candidates_strict();
    failures += test_scan_candidates_non_strict_probe();
    failures += test_scan_candidates_dedupes();

    if (failures == 0)
    {
        printf("All i2c helper tests PASSED.\n");
        return 0;
    }

    fprintf(stderr, "%d i2c helper test(s) FAILED.\n", failures);
    return 1;
}

