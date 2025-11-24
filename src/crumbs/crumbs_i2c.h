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

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_I2C_H */
