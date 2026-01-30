#ifndef CRUMBS_I2C_H
#define CRUMBS_I2C_H

#include <stddef.h>
#include <stdint.h>

/**
 * @file
 * @brief Lightweight HAL I²C primitive signatures used by CRUMBS adapters.
 */

/* ============================================================================
 * Timing Helper Macros
 * ============================================================================ */

/**
 * @brief Calculate elapsed milliseconds (wraparound-safe).
 *
 * Uses unsigned subtraction which handles 32-bit wraparound correctly.
 * Both platform timers wrap at ~49.7 days; this macro produces correct
 * results even when 'now' has wrapped past 'start'.
 *
 * @param start Start time from platform millis function.
 * @param now   Current time from platform millis function.
 * @return Elapsed milliseconds (correct even across wraparound).
 *
 * Example:
 * @code
 * uint32_t start = crumbs_arduino_millis();
 * // ... do work ...
 * uint32_t elapsed = CRUMBS_ELAPSED_MS(start, crumbs_arduino_millis());
 * @endcode
 */
#define CRUMBS_ELAPSED_MS(start, now) ((uint32_t)((now) - (start)))

/**
 * @brief Check if timeout has expired (wraparound-safe).
 *
 * @param start      Start time from platform millis function.
 * @param now        Current time from platform millis function.
 * @param timeout_ms Timeout duration in milliseconds.
 * @return Non-zero if timeout has expired, zero otherwise.
 *
 * Example:
 * @code
 * uint32_t start = crumbs_linux_millis();
 * while (!CRUMBS_TIMEOUT_EXPIRED(start, crumbs_linux_millis(), 1000)) {
 *     // Wait up to 1 second
 * }
 * @endcode
 */
#define CRUMBS_TIMEOUT_EXPIRED(start, now, timeout_ms) \
    (CRUMBS_ELAPSED_MS((start), (now)) >= (timeout_ms))

/* ============================================================================
 * I²C Function Signatures
 * ============================================================================ */

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

    /**
     * @brief Platform millisecond timer function signature.
     *
     * Implementations should return monotonic milliseconds since boot.
     * Use CRUMBS_ELAPSED_MS() or CRUMBS_TIMEOUT_EXPIRED() for wraparound-safe
     * timeout calculations.
     *
     * @return Milliseconds elapsed since boot/epoch.
     */
    typedef uint32_t (*crumbs_platform_millis_fn)(void);

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_I2C_H */
