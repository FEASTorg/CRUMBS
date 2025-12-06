/*
 * Test the crumbs_controller_scan_for_crumbs logic using fake read/write
 * implementations that emulate devices at specific addresses.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"

/* We'll emulate two CRUMBS devices at 0x08 and 0x10 */
#define DEV_A 0x08
#define DEV_B 0x10

static int fake_write(void *user_ctx, uint8_t addr, const uint8_t *data, size_t len)
{
    (void)user_ctx;
    (void)data;
    (void)len;
    /* accept writes for DEV_A and DEV_B */
    if (addr == DEV_A || addr == DEV_B)
        return 0;
    return -1;
}

static int fake_read(void *user_ctx, uint8_t addr, uint8_t *buffer, size_t len, uint32_t timeout_us)
{
    (void)user_ctx;
    (void)timeout_us;
    if (addr != DEV_A && addr != DEV_B)
        return 0; /* no data */

    /* Build a valid CRUMBS frame using encode helper */
    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = (uint8_t)addr; /* unique but arbitrary */
    m.command_type = 0x1;
    m.data_len = 3;
    m.data[0] = (uint8_t)addr;
    m.data[1] = 0xAA;
    m.data[2] = 0xBB;

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));
    if (w == 0 || w > len)
        return -1;

    memcpy(buffer, frame, w);
    return (int)w;
}

static int fake_read_noncrumbs(void *user_ctx, uint8_t addr, uint8_t *buf, size_t len, uint32_t to)
{
    (void)user_ctx;
    (void)to;
    if (addr != DEV_A)
        return 0;
    if (len < 5)
        return 0;
    /* write 5 bytes of garbage (not a valid CRUMBS frame) */
    for (size_t i = 0; i < 5 && i < len; ++i)
        buf[i] = (uint8_t)(i + 1);
    return 5;
}

static int test_scan_finds_devices(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    uint8_t found[16];

    int n = crumbs_controller_scan_for_crumbs(&ctx, 0x03, 0x20, 0 /* non-strict */,
                                              fake_write, fake_read, NULL, found, sizeof(found), 10000);

    if (n < 0)
    {
        fprintf(stderr, "scan_finds_devices: scan failed rc=%d\n", n);
        return 1;
    }

    /* Expect to find both devices */
    int sawA = 0, sawB = 0;
    for (int i = 0; i < n; ++i)
    {
        if (found[i] == DEV_A)
            sawA = 1;
        if (found[i] == DEV_B)
            sawB = 1;
    }

    if (!sawA || !sawB)
    {
        fprintf(stderr, "scan_finds_devices: did not find expected devices: sawA=%d sawB=%d n=%d\n",
                sawA, sawB, n);
        return 1;
    }

    printf("  scan finds devices: PASS\n");
    return 0;
}

static int test_scan_rejects_noncrumbs(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    uint8_t found[16];

    int n = crumbs_controller_scan_for_crumbs(&ctx, 0x03, 0x20, 0,
                                              fake_write, fake_read_noncrumbs, NULL, found, sizeof(found), 10000);
    if (n < 0)
    {
        fprintf(stderr, "scan_rejects_noncrumbs: scan failed rc=%d\n", n);
        return 1;
    }
    if (n != 0)
    {
        fprintf(stderr, "scan_rejects_noncrumbs: incorrectly identified non-CRUMBS device n=%d\n", n);
        return 1;
    }

    printf("  scan rejects non-CRUMBS: PASS\n");
    return 0;
}

static int test_scan_empty_range(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    uint8_t found[16];

    /* Scan a range with no devices */
    int n = crumbs_controller_scan_for_crumbs(&ctx, 0x50, 0x60, 0,
                                              fake_write, fake_read, NULL, found, sizeof(found), 10000);
    if (n < 0)
    {
        fprintf(stderr, "scan_empty_range: scan failed rc=%d\n", n);
        return 1;
    }
    if (n != 0)
    {
        fprintf(stderr, "scan_empty_range: should find 0 devices, got %d\n", n);
        return 1;
    }

    printf("  scan empty range: PASS\n");
    return 0;
}

int main(void)
{
    int failures = 0;

    printf("Running scan tests:\n");

    failures += test_scan_finds_devices();
    failures += test_scan_rejects_noncrumbs();
    failures += test_scan_empty_range();

    if (failures == 0)
    {
        printf("OK all scan tests passed\n");
        return 0;
    }
    else
    {
        fprintf(stderr, "FAILED %d test(s)\n", failures);
        return 1;
    }
}
