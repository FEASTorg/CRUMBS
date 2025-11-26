#ifndef CRUMBS_H
#define CRUMBS_H

#include <stddef.h>
#include <stdint.h>

#include "crumbs_message.h"
#include "crumbs_crc.h"
#include "crumbs_i2c.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /** @file
     * @brief CRUMBS core public API declarations.
     *
     * CRUMBS is a minimal fixed-size I²C message protocol. This header
     * declares the public types and functions used by both controller and
     * peripheral roles.
     */

    /**
     * @brief Role of a CRUMBS endpoint on the I²C bus.
     */
    typedef enum
    {
        CRUMBS_ROLE_CONTROLLER = 0,
        CRUMBS_ROLE_PERIPHERAL = 1
    } crumbs_role_t;

    struct crumbs_context_s;
    typedef struct crumbs_context_s crumbs_context_t;

    /**
     * @brief Called when a complete, CRC-valid message is received (peripheral).
     *
     * @param ctx Pointer to the active CRUMBS context.
     * @param msg Pointer to the decoded message (valid only for callback duration).
     */
    typedef void (*crumbs_message_cb_t)(
        crumbs_context_t *ctx,
        const crumbs_message_t *msg);

    /**
     * @brief Called when the bus master requests a reply from a peripheral.
     *
     * Callback must populate @p msg with the reply message.
     *
     * @param ctx Pointer to the active CRUMBS context.
     * @param msg Pointer to message object to fill with the reply.
     */
    typedef void (*crumbs_request_cb_t)(
        crumbs_context_t *ctx,
        crumbs_message_t *msg);

    /**
     * @brief State and configuration for a CRUMBS endpoint.
     *
     * This structure is plain-old-data so it is safe to allocate on the
     * stack or in static storage across supported platforms.
     */
    struct crumbs_context_s
    {
        uint8_t address;    /**< I²C address for peripheral role; 0 for controller. */
        crumbs_role_t role; /**< Controller or peripheral. */

        uint32_t crc_error_count; /**< Number of CRC failures seen during decode. */
        uint8_t last_crc_ok;      /**< Non-zero if the last decode had a valid CRC. */

        crumbs_message_cb_t on_message; /**< Called when a message is received (peripheral). */
        crumbs_request_cb_t on_request; /**< Called when the bus master requests a reply. */
        void *user_data;                /**< Opaque pointer for user code (forwarded to callbacks). */
    };

    /**
     * @brief Initialize a CRUMBS context.
     *
     * Hardware setup is the responsibility of the platform HAL.
     *
     * @param ctx Context to initialize (must not be NULL).
     * @param role Role for this endpoint.
     * @param address Peripheral I²C address (ignored for controller).
     */
    void crumbs_init(crumbs_context_t *ctx,
                     crumbs_role_t role,
                     uint8_t address);

    /**
     * @brief Install callbacks and user data for a context.
     *
     * Pass NULL for callbacks you do not need.
     */
    void crumbs_set_callbacks(crumbs_context_t *ctx,
                              crumbs_message_cb_t on_message,
                              crumbs_request_cb_t on_request,
                              void *user_data);

    /**
     * @brief Encode a message into the CRUMBS wire frame.
     *
     * Note: msg->slice_address is not serialized on the wire.
     *
     * @param msg Pointer to message to encode.
     * @param buffer Destination buffer.
     * @param buffer_len Size of @p buffer in bytes.
     * @return Number of bytes written (CRUMBS_MESSAGE_SIZE) or 0 on error.
     */
    size_t crumbs_encode_message(const crumbs_message_t *msg,
                                 uint8_t *buffer,
                                 size_t buffer_len);

    /**
     * @brief Decode a CRUMBS frame into a message object.
     *
     * Updates CRC-related statistics in @p ctx when provided.
     *
     * @param buffer Input buffer containing a serialized frame.
     * @param buffer_len Length in bytes of @p buffer.
     * @param msg Output message struct (must not be NULL).
     * @param ctx Optional context updated with CRC stats (may be NULL).
     * @return 0 on success, -1 if buffer too small, -2 on CRC mismatch.
     */
    int crumbs_decode_message(const uint8_t *buffer,
                              size_t buffer_len,
                              crumbs_message_t *msg,
                              crumbs_context_t *ctx);

    /**
     * @brief Send a CRUMBS message to a 7-bit I²C target (controller helper).
     *
     * @param ctx Initialized CRUMBS context in controller mode.
     * @param target_addr 7-bit I²C address of the peripheral.
     * @param msg Message to send.
     * @param write_fn I²C write function (crumbs_i2c_write_fn).
     * @param write_ctx Opaque pointer passed to @p write_fn (Wire*, linux handle, etc.).
     * @return 0 on success, non-zero on error.
     */
    int crumbs_controller_send(const crumbs_context_t *ctx,
                               uint8_t target_addr,
                               const crumbs_message_t *msg,
                               crumbs_i2c_write_fn write_fn,
                               void *write_ctx);

    /**
     * @brief Probe an I²C address range for CRUMBS-capable devices.
     *
     * The function attempts to read a CRUMBS frame and decode it. In
     * non-strict mode a small probe write may be issued to stimulate a reply.
     *
     * @param ctx Optional controller context pointer (may be NULL).
     * @param start_addr Address range start (inclusive).
     * @param end_addr Address range end (inclusive).
     * @param strict Non-zero to require a strict read; 0 to use a probe write.
     * @param write_fn Optional write function used to stimulate replies.
     * @param read_fn Read function to use for data-phase reads.
     * @param io_ctx Opaque I/O context forwarded to read/write callbacks.
     * @param found Output buffer to receive discovered addresses.
     * @param max_found Capacity of @p found buffer.
     * @param timeout_us Read timeout hint in microseconds.
     * @return Number of discovered devices (>=0) or negative on error.
     */
    int crumbs_controller_scan_for_crumbs(const crumbs_context_t *ctx,
                                          uint8_t start_addr,
                                          uint8_t end_addr,
                                          int strict,
                                          crumbs_i2c_write_fn write_fn,
                                          crumbs_i2c_read_fn read_fn,
                                          void *io_ctx,
                                          uint8_t *found,
                                          size_t max_found,
                                          uint32_t timeout_us);

    /**
     * @brief Process raw bytes received by a peripheral HAL.
     *
     * HALs should pass the raw frame bytes into this function so the core can
     * decode them and invoke on_message() when valid.
     *
     * @param ctx Active CRUMBS context (peripheral role).
     * @param buffer Raw bytes received.
     * @param len Number of bytes in @p buffer.
     * @return 0 on success, negative on decode error.
     */
    int crumbs_peripheral_handle_receive(crumbs_context_t *ctx,
                                         const uint8_t *buffer,
                                         size_t len);

    /**
     * @brief Build an encoded reply frame for use inside an I²C request handler.
     *
     * If no on_request callback is configured the function returns success with
     * *out_len set to 0.
     *
     * @param ctx Active CRUMBS context (peripheral role).
     * @param out_buf Buffer to receive encoded frame.
     * @param out_buf_len Length of @p out_buf in bytes.
     * @param out_len On success, set to the encoded byte count (may be 0).
     * @return 0 on success, negative on error.
     */
    int crumbs_peripheral_build_reply(crumbs_context_t *ctx,
                                      uint8_t *out_buf,
                                      size_t out_buf_len,
                                      size_t *out_len);

    /* CRC stats helpers */
    uint32_t crumbs_get_crc_error_count(const crumbs_context_t *ctx);
    int crumbs_last_crc_ok(const crumbs_context_t *ctx);
    void crumbs_reset_crc_stats(crumbs_context_t *ctx);

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_H */
