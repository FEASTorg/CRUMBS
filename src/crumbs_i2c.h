#ifndef CRUMBS_I2C_H
#define CRUMBS_I2C_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file
 * @brief Lightweight HAL I²C primitive signatures used by CRUMBS adapters.
 */

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief I²C write primitive signature used by controller helpers.
     *
     * Implementations should perform a START + address(w) + data + STOP.
     *
     * @param user_ctx Opaque implementation pointer (e.g., TwoWire* or linux handle).
     * @param addr 7-bit I²C address.
     * @param data Pointer to bytes to send.
     * @param len Number of bytes to send.
     * @return 0 on success, non-zero on error.
     */
    typedef int (*crumbs_i2c_write_fn)(
        void *user_ctx,
        uint8_t addr,
        const uint8_t *data,
        size_t len);

    /**
     * @brief Simple I2C bus scanner helper signature.
     *
     * Implementations should probe addresses in the inclusive range
     * [start_addr, end_addr] and write responsive addresses into @p found.
     *
     * @param user_ctx   Opaque implementation pointer.
     * @param start_addr First address to probe (inclusive).
     * @param end_addr   Last address to probe (inclusive).
     * @param strict     Non-zero for strict probing (write-data-phase ACK).
     * @param found      Output buffer for discovered addresses.
     * @param max_found  Capacity of @p found buffer.
     * @return Number of found addresses on success, negative on error.
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
     * Implementations should read up to @p len bytes from the peripheral.
     *
     * @param user_ctx   Opaque implementation pointer.
     * @param addr       7-bit I²C address of peripheral.
     * @param buffer     Output buffer for received bytes.
     * @param len        Maximum bytes to read.
     * @param timeout_us Timeout hint in microseconds (0 = no wait).
     * @return Number of bytes read (>=0) on success, negative on error.
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
