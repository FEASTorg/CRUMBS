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
     * @brief Called when a complete, CRC-valid message is received by a peripheral.
     *
     * @param ctx  Pointer to the active CRUMBS context.
     * @param msg  Pointer to the decoded message (valid during callback).
     */
    typedef void (*crumbs_message_cb_t)(
        crumbs_context_t *ctx,
        const crumbs_message_t *msg);

    /**
     * @brief Called when the bus master requests data from a peripheral.
     *
     * The callback should populate @p msg with the reply to be sent.
     *
     * @param ctx  Pointer to the active CRUMBS context.
     * @param msg  Pointer to a message to be filled as the reply.
     */
    typedef void (*crumbs_request_cb_t)(
        crumbs_context_t *ctx,
        crumbs_message_t *msg);

    /**
     * @brief State and configuration for a single CRUMBS endpoint.
     *
     * This is intentionally small and POD so it can live on the stack
     * or in static storage on both Arduino and Linux.
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
     * Does not touch any underlying hardware. HALs (Arduino, linux-wire, etc.)
     * should perform their own I²C setup separately.
     *
     * @param ctx     Context to initialize.
     * @param role    Role of this endpoint.
     * @param address Peripheral I²C address (ignored in controller mode).
     */
    void crumbs_init(crumbs_context_t *ctx,
                     crumbs_role_t role,
                     uint8_t address);

    /**
     * @brief Install callbacks and user_data for a context.
     *
     * All fields are optional; pass NULL for callbacks you don't use.
     */
    void crumbs_set_callbacks(crumbs_context_t *ctx,
                              crumbs_message_cb_t on_message,
                              crumbs_request_cb_t on_request,
                              void *user_data);

    /**
     * @brief Encode a message into a flat byte buffer using the CRUMBS wire format.
     *
     * The slice_address field is *not* serialized.
     *
     * @param msg        Pointer to message to encode.
     * @param buffer     Destination buffer.
     * @param buffer_len Length of @p buffer in bytes.
     * @return Number of bytes written on success (CRUMBS_MESSAGE_SIZE), 0 on error.
     */
    size_t crumbs_encode_message(const crumbs_message_t *msg,
                                 uint8_t *buffer,
                                 size_t buffer_len);

    /**
     * @brief Decode a message from a flat byte buffer.
     *
     * On success, fills @p msg and updates CRC stats in @p ctx (if non-NULL).
     *
     * @param buffer     Pointer to serialized frame.
     * @param buffer_len Length of @p buffer in bytes.
     * @param msg        Output message struct.
     * @param ctx        Optional context for updating CRC stats (may be NULL).
     * @return 0 on success;
     *         -1 if buffer too small;
     *         -2 if CRC mismatch.
     */
    int crumbs_decode_message(const uint8_t *buffer,
                              size_t buffer_len,
                              crumbs_message_t *msg,
                              crumbs_context_t *ctx);

    /**
     * @brief Convenience helper for controller: send a message over I²C.
     *
     * This is a thin adapter around a platform-provided write primitive.
     *
     * @param ctx        Initialized CRUMBS context in controller mode.
     * @param target_addr 7-bit I²C address of the peripheral.
     * @param msg        Message to send.
     * @param write_fn   I²C write function (see crumbs_i2c_write_fn).
     * @param write_ctx  Opaque pointer passed to @p write_fn (Wire*, linux bus handle, etc.).
     * @return 0 on success, non-zero on error.
     */
    int crumbs_controller_send(const crumbs_context_t *ctx,
                               uint8_t target_addr,
                               const crumbs_message_t *msg,
                               crumbs_i2c_write_fn write_fn,
                               void *write_ctx);

    /**
     * @brief Peripheral-side entry point for raw bytes received over I²C.
     *
     * HALs (e.g. Arduino's onReceive callback, linux-wire read handler) should call
     * this with the bytes read from the bus. On success, the on_message() callback
     * (if configured) is invoked.
     *
     * @param ctx    Active CRUMBS context (peripheral role).
     * @param buffer Pointer to raw bytes received.
     * @param len    Number of bytes in @p buffer.
     * @return 0 on success; negative on decode error.
     */
    int crumbs_peripheral_handle_receive(crumbs_context_t *ctx,
                                         const uint8_t *buffer,
                                         size_t len);

    /**
     * @brief Peripheral-side helper for generating a reply frame on request.
     *
     * HALs should call this inside their I²C request callback to obtain the
     * next reply frame (if any) to send over the bus.
     *
     * If no on_request callback is configured, @p out_len is set to 0 and
     * 0 is returned.
     *
     * @param ctx       Active CRUMBS context (peripheral role).
     * @param out_buf   Destination buffer for the encoded reply frame.
     * @param out_buf_len Size of @p out_buf.
     * @param out_len   On success, number of bytes to send (may be 0).
     * @return 0 on success; negative on error.
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
