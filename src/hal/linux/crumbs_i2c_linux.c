/**
 * @file
 * @brief Linux HAL using linux-wire for CRUMBS (platform native I²C helpers).
 *
 * This file provides native Linux implementations when compiled on Linux
 * and stubbed fallbacks for other platforms so the whole repo can be
 * included in non-Linux builds.
 */

#include "crumbs_linux.h"

#include <string.h> /* memset */
#include <errno.h>

#include "crumbs_crc.h" /* for CRUMBS_MESSAGE_SIZE, etc., via crumbs.h tree */

#if defined(__linux__)

#include <linux_wire.h> /* linux-wire C API */

/* ---- Public API -------------------------------------------------------- */

int crumbs_linux_init_controller(crumbs_context_t *ctx,
                                 crumbs_linux_i2c_t *i2c,
                                 const char *device_path,
                                 uint32_t timeout_us)
{
    if (!ctx || !i2c || !device_path || device_path[0] == '\0')
    {
        return -1;
    }

    memset(i2c, 0, sizeof(*i2c));

    /* Initialize CRUMBS context as controller. Address unused in this role. */
    crumbs_init(ctx, CRUMBS_ROLE_CONTROLLER, 0u);

    /* Open the Linux I²C bus. */
    if (lw_open_bus(&i2c->bus, device_path) != 0)
    {
        /* lw_open_bus already sets errno and bus.fd = -1 on error. */
        return -2;
    }

    /* Optional timeout hint (informational in linux-wire). */
    if (timeout_us > 0u)
    {
        lw_set_timeout(&i2c->bus, timeout_us);
    }

    return 0;
}

void crumbs_linux_close(crumbs_linux_i2c_t *i2c)
{
    if (!i2c)
    {
        return;
    }

    lw_close_bus(&i2c->bus);
    memset(i2c, 0, sizeof(*i2c));
}

int crumbs_linux_i2c_write(void *user_ctx,
                           uint8_t target_addr,
                           const uint8_t *data,
                           size_t len)
{
    if (!user_ctx || !data || len == 0u)
    {
        return -1;
    }

    crumbs_linux_i2c_t *i2c = (crumbs_linux_i2c_t *)user_ctx;
    lw_i2c_bus *bus = &i2c->bus;

    if (bus->fd < 0)
    {
        return -1;
    }

    /* Select the slave. */
    if (lw_set_slave(bus, target_addr) != 0)
    {
        return -2;
    }

    /* Single write with STOP at the end (send_stop = 1). */
    ssize_t written = lw_write(bus, data, len, 1);
    if (written < 0)
    {
        return -3;
    }

    if ((size_t)written != len)
    {
        return -4;
    }

    return 0;
}

int crumbs_linux_read_message(crumbs_linux_i2c_t *i2c,
                              uint8_t target_addr,
                              crumbs_context_t *ctx,
                              crumbs_message_t *out_msg)
{
    if (!i2c || !out_msg)
    {
        return -1;
    }

    lw_i2c_bus *bus = &i2c->bus;
    if (bus->fd < 0)
    {
        return -1;
    }

    if (lw_set_slave(bus, target_addr) != 0)
    {
        return -2;
    }

    /* Expect a fixed-size CRUMBS frame; loop until no more data or buffer fills. */
    uint8_t buf[CRUMBS_MESSAGE_SIZE];
    size_t total = 0u;

    while (total < sizeof(buf))
    {
        ssize_t r = lw_read(bus, buf + total, sizeof(buf) - total);
        if (r < 0)
        {
            /* Low-level I/O error. */
            return -3;
        }
        if (r == 0)
        {
            /* No more data. */
            break;
        }
        total += (size_t)r;
    }

    if (total == 0u)
    {
        /* No bytes read. */
        return -4;
    }

    /* Decode with CRUMBS core -- validates CRC and updates ctx stats. */
    int rc = crumbs_decode_message(buf, total, out_msg, ctx);
    return rc; /* 0 on success, <0 on decode/CRC error */
}

int crumbs_linux_scan(void *user_ctx,
                      uint8_t start_addr,
                      uint8_t end_addr,
                      int strict,
                      uint8_t *found,
                      size_t max_found)
{
    if (!user_ctx || !found || max_found == 0u)
    {
        return -1; /* invalid args */
    }

    crumbs_linux_i2c_t *i2c = (crumbs_linux_i2c_t *)user_ctx;
    lw_i2c_bus *bus = &i2c->bus;
    if (bus->fd < 0)
    {
        return -1; /* bus not open */
    }

    size_t count = 0u;
    uint8_t dummy = 0u;

    for (int addr = start_addr; addr <= end_addr; ++addr)
    {
        /* Select the slave address; skip if selection fails. */
        if (lw_set_slave(bus, (uint8_t)addr) != 0)
            continue;

        if (strict)
        {
            /* Strict probe: attempt a small read and treat positive results as present. */
            ssize_t r = lw_read(bus, &dummy, 1);
            if (r > 0)
            {
                if (count < max_found)
                    found[count] = (uint8_t)addr;
                ++count;
            }
        }
        else
        {
            /* Non-strict probe: perform a zero-length write (address-only) and treat
               any non-negative return as success (address ACK). */
            ssize_t w = lw_write(bus, NULL, 0, 1);
            if (w >= 0)
            {
                if (count < max_found)
                    found[count] = (uint8_t)addr;
                ++count;
            }
        }

        if (count >= max_found)
            break;
    }

    return (int)count;
}

int crumbs_linux_read(void *user_ctx,
                      uint8_t addr,
                      uint8_t *buffer,
                      size_t len,
                      uint32_t timeout_us)
{
    if (!user_ctx || !buffer || len == 0u)
        return -1;

    crumbs_linux_i2c_t *i2c = (crumbs_linux_i2c_t *)user_ctx;
    lw_i2c_bus *bus = &i2c->bus;
    if (bus->fd < 0)
        return -1;

    if (timeout_us > 0u)
        lw_set_timeout(bus, timeout_us);

    if (lw_set_slave(bus, addr) != 0)
        return -2;

    size_t total = 0u;
    while (total < len)
    {
        ssize_t r = lw_read(bus, buffer + total, len - total);
        if (r < 0)
            return -3;
        if (r == 0)
            break;
        total += (size_t)r;
    }

    return (int)total;
}

#else /* non-Linux builds */

/* Stubs for non-Linux builds (Arduino/embedded). They return errors so
 * callers on those platforms fail cleanly instead of triggering a compile
 * or link-time error. This keeps the file present in the Arduino package
 * without bringing a Linux-only dependency into Arduino builds.
 */

int crumbs_linux_init_controller(crumbs_context_t *ctx,
                                 crumbs_linux_i2c_t *i2c,
                                 const char *device_path,
                                 uint32_t timeout_us)
{
    (void)ctx;
    (void)i2c;
    (void)device_path;
    (void)timeout_us;
    return -1; /* not supported on this platform */
}

void crumbs_linux_close(crumbs_linux_i2c_t *i2c)
{
    (void)i2c;
}

int crumbs_linux_i2c_write(void *user_ctx,
                           uint8_t target_addr,
                           const uint8_t *data,
                           size_t len)
{
    (void)user_ctx;
    (void)target_addr;
    (void)data;
    (void)len;
    return -1;
}

int crumbs_linux_read_message(crumbs_linux_i2c_t *i2c,
                              uint8_t target_addr,
                              crumbs_context_t *ctx,
                              crumbs_message_t *out_msg)
{
    (void)i2c;
    (void)target_addr;
    (void)ctx;
    (void)out_msg;
    return -1;
}

int crumbs_linux_scan(void *user_ctx,
                      uint8_t start_addr,
                      uint8_t end_addr,
                      int strict,
                      uint8_t *found,
                      size_t max_found)
{
    (void)user_ctx;
    (void)start_addr;
    (void)end_addr;
    (void)strict;
    (void)found;
    (void)max_found;
    return -1; /* not supported on this platform */
}

int crumbs_linux_read(void *user_ctx,
                      uint8_t addr,
                      uint8_t *buffer,
                      size_t len,
                      uint32_t timeout_us)
{
    (void)user_ctx;
    (void)addr;
    (void)buffer;
    (void)len;
    (void)timeout_us;
    return -1;
}

#endif /* defined(__linux__) */
