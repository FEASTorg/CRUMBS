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

    if (len < CRUMBS_MESSAGE_SIZE)
        return 0;

    /* Build a valid CRUMBS frame using encode helper */
    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = (uint8_t)addr; /* unique but arbitrary */
    m.command_type = 0x1;
    m.data[0] = (float)addr;

    uint8_t frame[CRUMBS_MESSAGE_SIZE];
    size_t w = crumbs_encode_message(&m, frame, sizeof(frame));
    if (w == 0)
        return -1;

    memcpy(buffer, frame, w);
    return (int)w;
}

int main(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    uint8_t found[16];

    int n = crumbs_controller_scan_for_crumbs(&ctx, 0x03, 0x20, 0 /* non-strict */,
                                              fake_write, fake_read, NULL, found, sizeof(found), 10000);

    if (n < 0)
    {
        fprintf(stderr, "scan failed rc=%d\n", n);
        return 2;
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
        fprintf(stderr, "scan did not find expected devices: sawA=%d sawB=%d n=%d\n", sawA, sawB, n);
        return 3;
    }

    /* Also test that a non-CRUMBS payload is not accepted */
    /* implement fake_read_noncrumbs returning invalid payload */
    int fake_read_noncrumbs(void *user_ctx, uint8_t addr, uint8_t *buf, size_t len, uint32_t to)
    {
        (void)user_ctx;
        (void)to;
        if (addr != DEV_A)
            return 0;
        if (len < 5)
            return 0;
        /* write 5 bytes of garbage (not CRUMBS frame) */
        for (size_t i = 0; i < 5 && i < len; ++i)
            buf[i] = (uint8_t)(i + 1);
        return 5;
    }

    int n2 = crumbs_controller_scan_for_crumbs(&ctx, 0x03, 0x20, 0,
                                               fake_write, fake_read_noncrumbs, NULL, found, sizeof(found), 10000);
    if (n2 < 0)
    {
        fprintf(stderr, "scan2 failed rc=%d\n", n2);
        return 4;
    }
    if (n2 != 0)
    {
        fprintf(stderr, "scan incorrectly identified non-CRUMBS device n2=%d\n", n2);
        return 5;
    }

    printf("OK scan tests\n");
    return 0;
}
