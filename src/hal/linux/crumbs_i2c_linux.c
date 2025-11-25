#include "crumbs_linux.h"

#include <string.h> /* memset */
#include <errno.h>

#include "crumbs_crc.h" /* for CRUMBS_MESSAGE_SIZE, etc., via crumbs.h tree */

#include <linux_wire.h> /* linux-wire C API */

/* ---- Internal opaque struct definition -------------------------------- */

struct crumbs_linux_i2c_s
{
    lw_i2c_bus bus;
};

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

    /* Initialize CRUMBS context as controller. Address is unused in this role. */
    crumbs_init(ctx, CRUMBS_ROLE_CONTROLLER, 0u);

    /* Open the Linux I2C bus. */
    if (lw_open_bus(&i2c->bus, device_path) != 0)
    {
        /* lw_open_bus already sets errno and bus.fd = -1 on error. */
        return -2;
    }

    /* Optional timeout hint. Currently informational in linux-wire. */
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

    /* We expect a fixed-size CRUMBS frame, but we defensively loop until
       read() returns 0 or error, or the buffer is full. */
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
        /* Nothing received. */
        return -4;
    }

    /* Decode using the shared CRUMBS core. This validates CRC and updates
       CRC stats in ctx (if non-NULL). */
    int rc = crumbs_decode_message(buf, total, out_msg, ctx);
    return rc; /* 0 on success, <0 on decode/CRC error */
}
