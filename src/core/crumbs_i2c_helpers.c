/**
 * @file
 * @brief Raw I2C helper utilities bound to crumbs_device_t transports.
 */

#include "crumbs.h"

#include <string.h> /* memcpy */

#ifndef CRUMBS_I2C_DEV_MAX_WRITE
#define CRUMBS_I2C_DEV_MAX_WRITE 64u
#endif

int crumbs_i2c_dev_write(const crumbs_device_t *dev,
                         const uint8_t *data,
                         size_t len)
{
    if (!dev || !dev->write_fn)
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if (len == 0u)
    {
        return CRUMBS_I2C_DEV_OK;
    }

    if (!data)
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if (dev->write_fn(dev->io, dev->addr, data, len) != 0)
    {
        return CRUMBS_I2C_DEV_E_WRITE;
    }

    return CRUMBS_I2C_DEV_OK;
}

int crumbs_i2c_dev_read(const crumbs_device_t *dev,
                        uint8_t *data,
                        size_t len,
                        uint32_t timeout_us)
{
    if (!dev || !dev->read_fn)
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if (len == 0u)
    {
        return CRUMBS_I2C_DEV_OK;
    }

    if (!data)
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    int n = dev->read_fn(dev->io, dev->addr, data, len, timeout_us);
    if (n < 0)
    {
        return CRUMBS_I2C_DEV_E_READ;
    }

    if ((size_t)n != len)
    {
        return CRUMBS_I2C_DEV_E_SHORT_READ;
    }

    return CRUMBS_I2C_DEV_OK;
}

int crumbs_i2c_dev_write_then_read(const crumbs_device_t *dev,
                                   const uint8_t *tx,
                                   size_t tx_len,
                                   uint8_t *rx,
                                   size_t rx_len,
                                   uint32_t timeout_us,
                                   int require_repeated_start,
                                   crumbs_i2c_write_read_fn write_read_fn)
{
    if (!dev)
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if ((tx_len > 0u && !tx) || (rx_len > 0u && !rx))
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if (write_read_fn)
    {
        int n = write_read_fn(dev->io, dev->addr, tx, tx_len, rx, rx_len,
                              timeout_us, require_repeated_start);
        if (n == CRUMBS_I2C_DEV_E_NO_REPEATED_START)
        {
            return CRUMBS_I2C_DEV_E_NO_REPEATED_START;
        }
        if (n < 0)
        {
            return CRUMBS_I2C_DEV_E_READ;
        }
        if ((size_t)n != rx_len)
        {
            return CRUMBS_I2C_DEV_E_SHORT_READ;
        }
        return CRUMBS_I2C_DEV_OK;
    }

    if (require_repeated_start)
    {
        return CRUMBS_I2C_DEV_E_NO_REPEATED_START;
    }

    if (tx_len > 0u)
    {
        int wr = crumbs_i2c_dev_write(dev, tx, tx_len);
        if (wr != CRUMBS_I2C_DEV_OK)
        {
            return wr;
        }
    }

    if (rx_len > 0u)
    {
        int rr = crumbs_i2c_dev_read(dev, rx, rx_len, timeout_us);
        if (rr != CRUMBS_I2C_DEV_OK)
        {
            return rr;
        }
    }

    return CRUMBS_I2C_DEV_OK;
}

int crumbs_i2c_dev_read_reg_ex(const crumbs_device_t *dev,
                               const uint8_t *reg,
                               size_t reg_len,
                               uint8_t *out,
                               size_t out_len,
                               uint32_t timeout_us,
                               int require_repeated_start,
                               crumbs_i2c_write_read_fn write_read_fn)
{
    if ((reg_len > 0u && !reg) || (out_len > 0u && !out))
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    return crumbs_i2c_dev_write_then_read(dev, reg, reg_len, out, out_len,
                                          timeout_us, require_repeated_start,
                                          write_read_fn);
}

int crumbs_i2c_dev_write_reg_ex(const crumbs_device_t *dev,
                                const uint8_t *reg,
                                size_t reg_len,
                                const uint8_t *data,
                                size_t data_len)
{
    if (!dev || !dev->write_fn)
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if ((reg_len > 0u && !reg) || (data_len > 0u && !data))
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if (reg_len == 0u && data_len == 0u)
    {
        return CRUMBS_I2C_DEV_E_INVALID;
    }

    if (reg_len == 0u)
    {
        return crumbs_i2c_dev_write(dev, data, data_len);
    }

    if (data_len == 0u)
    {
        return crumbs_i2c_dev_write(dev, reg, reg_len);
    }

    if (reg_len > (SIZE_MAX - data_len))
    {
        return CRUMBS_I2C_DEV_E_SIZE;
    }

    size_t total = reg_len + data_len;
    if (total > CRUMBS_I2C_DEV_MAX_WRITE)
    {
        return CRUMBS_I2C_DEV_E_SIZE;
    }

    uint8_t frame[CRUMBS_I2C_DEV_MAX_WRITE];
    memcpy(frame, reg, reg_len);
    memcpy(frame + reg_len, data, data_len);

    return crumbs_i2c_dev_write(dev, frame, total);
}

int crumbs_i2c_dev_read_reg_u8(const crumbs_device_t *dev,
                               uint8_t reg,
                               uint8_t *out,
                               size_t out_len,
                               uint32_t timeout_us,
                               int require_repeated_start,
                               crumbs_i2c_write_read_fn write_read_fn)
{
    return crumbs_i2c_dev_read_reg_ex(dev, &reg, 1u, out, out_len,
                                      timeout_us, require_repeated_start,
                                      write_read_fn);
}

int crumbs_i2c_dev_write_reg_u8(const crumbs_device_t *dev,
                                uint8_t reg,
                                const uint8_t *data,
                                size_t data_len)
{
    return crumbs_i2c_dev_write_reg_ex(dev, &reg, 1u, data, data_len);
}

int crumbs_i2c_dev_read_reg_u16be(const crumbs_device_t *dev,
                                  uint16_t reg,
                                  uint8_t *out,
                                  size_t out_len,
                                  uint32_t timeout_us,
                                  int require_repeated_start,
                                  crumbs_i2c_write_read_fn write_read_fn)
{
    uint8_t reg_buf[2];
    reg_buf[0] = (uint8_t)((reg >> 8) & 0xFFu);
    reg_buf[1] = (uint8_t)(reg & 0xFFu);
    return crumbs_i2c_dev_read_reg_ex(dev, reg_buf, sizeof(reg_buf),
                                      out, out_len, timeout_us,
                                      require_repeated_start, write_read_fn);
}

int crumbs_i2c_dev_write_reg_u16be(const crumbs_device_t *dev,
                                   uint16_t reg,
                                   const uint8_t *data,
                                   size_t data_len)
{
    uint8_t reg_buf[2];
    reg_buf[0] = (uint8_t)((reg >> 8) & 0xFFu);
    reg_buf[1] = (uint8_t)(reg & 0xFFu);
    return crumbs_i2c_dev_write_reg_ex(dev, reg_buf, sizeof(reg_buf), data, data_len);
}

