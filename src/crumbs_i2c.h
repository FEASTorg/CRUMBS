#ifndef CRUMBS_I2C_H
#define CRUMBS_I2C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Signature for a simple "write a frame" I²C primitive.
     *
     * Implementations should perform:
     *   START + address(w) + data[0..len-1] + STOP
     *
     * Return 0 on success, non-zero on error.
     *
     * @param user_ctx  Opaque pointer for the implementation (e.g., Wire*, linux handle).
     * @param addr      7-bit I²C address.
     * @param data      Pointer to bytes to send.
     * @param len       Number of bytes to send.
     */
    typedef int (*crumbs_i2c_write_fn)(
        void *user_ctx,
        uint8_t addr,
        const uint8_t *data,
        size_t len);

    /**
     * @brief Simple I2C bus scanner helper signature.
     *
     * Implementations should probe addresses in the inclusive range [start_addr, end_addr]
     * and write responsive 7-bit addresses into @p found (up to @p max_found entries).
     *
     * The @p strict flag selects stricter probing (e.g., perform a write-data-phase) vs
     * a lighter probe that accepts address-only ACKs. Return value should be the number
     * of found addresses on success, or a negative error code.
     */
    typedef int (*crumbs_i2c_scan_fn)(
        void *user_ctx,
        uint8_t start_addr,
        uint8_t end_addr,
        int strict,
        uint8_t *found,
        size_t max_found);

    /**
     * @brief Simple I2C read primitive signature for HALs.
     *
     * Implementations should attempt to read up to @p len bytes from the
     * peripheral at @p addr into @p buffer. Implementations may honor
     * @p timeout_us as a hint for how long to wait for data.
     *
     * Return value: number of bytes read on success (>=0). Negative values
     * indicate an error.
     */
    typedef int (*crumbs_i2c_read_fn)(
        void *user_ctx,
        uint8_t addr,
        uint8_t *buffer,
        size_t len,
        uint32_t timeout_us);

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_I2C_H */
