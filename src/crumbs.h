#ifndef CRUMBS_H
#define CRUMBS_H

#include <stddef.h>
#include <stdint.h>

#include "crumbs_version.h"
#include "crumbs_message.h"
#include "crumbs_crc.h"
#include "crumbs_i2c.h"

/* ============================================================================
 * Debug Configuration
 * ============================================================================ */

/**
 * @brief Enable CRUMBS debug output.
 *
 * Define CRUMBS_DEBUG before including crumbs.h to enable debug messages.
 * You must also define CRUMBS_DEBUG_PRINT to specify how to output debug.
 *
 * Example for Arduino:
 *   #define CRUMBS_DEBUG
 *   #define CRUMBS_DEBUG_PRINT(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
 *
 * Example for Linux/stdio:
 *   #define CRUMBS_DEBUG
 *   #define CRUMBS_DEBUG_PRINT(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
 *
 * When CRUMBS_DEBUG is not defined, all debug calls compile to nothing.
 */
#ifdef CRUMBS_DEBUG
#ifndef CRUMBS_DEBUG_PRINT
/* Default: no-op if user didn't define a print function */
#define CRUMBS_DEBUG_PRINT(fmt, ...) ((void)0)
#endif
#define CRUMBS_DBG(fmt, ...) CRUMBS_DEBUG_PRINT("[CRUMBS] " fmt, ##__VA_ARGS__)
#else
#define CRUMBS_DBG(fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    /** @file
     * @brief CRUMBS core public API declarations.
     *
     * CRUMBS is a minimal variable-length I²C message protocol. This header
     * declares the public types and functions used by both controller and
     * peripheral roles.
     */

    /**
     * @brief Maximum number of command handlers that can be registered.
     *
     * Define this before including crumbs.h to adjust memory usage.
     * Default is 16 which balances memory use and typical needs.
     *
     * Memory usage: CRUMBS_MAX_HANDLERS * (sizeof(void*) * 2 + 2) bytes
     * - 16 handlers: ~68 bytes on AVR, ~132 bytes on 32-bit
     * - 8 handlers: ~36 bytes on AVR, ~68 bytes on 32-bit
     * - 32 handlers: ~132 bytes on AVR, ~260 bytes on 32-bit
     *
     * Dispatch always uses O(n) linear search for portability.
     * Set to 0 to disable handler dispatch entirely.
     *
     * IMPORTANT: For Arduino/PlatformIO, you must add this to your
     * platformio.ini build_flags to affect the library compilation:
     *   build_flags = -DCRUMBS_MAX_HANDLERS=8
     *
     * Defining it only in your sketch does NOT work because Arduino
     * precompiles the library separately.
     */
#ifndef CRUMBS_MAX_HANDLERS
#define CRUMBS_MAX_HANDLERS 16
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
    /**
     * @typedef crumbs_context_t
     * @brief Opaque handle type for a CRUMBS endpoint context (see struct crumbs_context_s).
     */
    typedef struct crumbs_context_s crumbs_context_t;

    /**
     * @brief Called when a complete, CRC-valid message is received (peripheral).
     *
     * @param ctx Pointer to the active CRUMBS context.
     * @param msg Pointer to the decoded message (valid only for callback duration).
     */
    typedef void (*crumbs_message_cb_t)(
        struct crumbs_context_s *ctx,
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
        struct crumbs_context_s *ctx,
        crumbs_message_t *msg);

    /**
     * @brief Command handler function type for dispatch-based message handling.
     *
     * Handlers are registered per opcode and called when a message with
     * that command is received. This provides an alternative to the on_message
     * callback for command-specific processing.
     *
     * @param ctx Pointer to the active CRUMBS context.
     * @param opcode The command type that triggered this handler.
     * @param data Pointer to the payload bytes (may be NULL if data_len==0).
     * @param data_len Number of payload bytes (0–27).
     * @param user_data Opaque pointer registered with the handler.
     */
    typedef void (*crumbs_handler_fn)(
        struct crumbs_context_s *ctx,
        uint8_t opcode,
        const uint8_t *data,
        uint8_t data_len,
        void *user_data);

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

#if CRUMBS_MAX_HANDLERS > 0
        /** @name Command Handler Dispatch Table
         *  Per-opcode handler functions and associated user data.
         *  Size controlled by CRUMBS_MAX_HANDLERS (default 16).
         *  Uses linear search for dispatch (O(n) but portable/safe).
         *  @{ */
        uint8_t handler_count;                             /**< Number of registered handlers. */
        uint8_t handler_opcode[CRUMBS_MAX_HANDLERS];       /**< Opcode for each handler slot. */
        crumbs_handler_fn handlers[CRUMBS_MAX_HANDLERS];   /**< Handler functions. */
        void *handler_userdata[CRUMBS_MAX_HANDLERS];     /**< User data for each handler. */
                                                         /** @} */
#endif                                                   /* CRUMBS_MAX_HANDLERS > 0 */
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
     * @brief Get the size of crumbs_context_t as compiled in the library.
     *
     * This can be used to verify that the user's context size matches the
     * library's expected size. On Arduino, a mismatch occurs when the user
     * defines CRUMBS_MAX_HANDLERS in their sketch instead of build_flags.
     *
     * Usage:
     * @code
     * if (sizeof(crumbs_context_t) != crumbs_context_size()) {
     *     // ERROR: CRUMBS_MAX_HANDLERS mismatch!
     * }
     * @endcode
     *
     * @return Size of crumbs_context_t in bytes as seen by the library.
     */
    size_t crumbs_context_size(void);

    /** @name Command Handler Registration
     *  Register/unregister per-command handlers for dispatch-based processing.
     *  Maximum handlers controlled by CRUMBS_MAX_HANDLERS (default 16).
     *  @{ */

    /**
     * @brief Register a handler for a specific command type.
     *
     * The handler will be invoked when a message with the given opcode
     * is received (after on_message, if configured). Registering a handler
     * for a opcode that already has one will overwrite the previous.
     *
     * @param ctx Context to register the handler on.
     * @param opcode The command type to handle (0–255).
     * @param fn Handler function to call (NULL to unregister).
     * @param user_data Opaque pointer passed to the handler when invoked.
     * @return 0 on success, -1 if ctx is NULL or handler table is full.
     */
    int crumbs_register_handler(crumbs_context_t *ctx,
                                uint8_t opcode,
                                crumbs_handler_fn fn,
                                void *user_data);

    /**
     * @brief Unregister a handler for a specific command type.
     *
     * Equivalent to crumbs_register_handler(ctx, opcode, NULL, NULL).
     *
     * @param ctx Context to unregister from.
     * @param opcode The command type to unregister.
     * @return 0 on success, -1 if ctx is NULL.
     */
    int crumbs_unregister_handler(crumbs_context_t *ctx,
                                  uint8_t opcode);

    /** @} */

    /**
     * @brief Encode a message into the CRUMBS wire frame.
     *
     * Note: msg->address is not serialized on the wire.
     *
     * @param msg Pointer to message to encode.
     * @param buffer Destination buffer.
     * @param buffer_len Size of @p buffer in bytes.
     * @return Number of bytes written (4 + data_len) or 0 on error.
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
     * @param buffer_len Length in bytes of @p buffer (must be 4 + data_len).
     * @param msg Output message struct (must not be NULL).
     * @param ctx Optional context updated with CRC stats (may be NULL).
     * @return 0 on success, -1 if buffer too small or invalid, -2 on CRC mismatch.
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

    /** @name CRC statistics helpers
     *  Convenience helpers to access / reset CRC statistics stored in a context.
     *  @{ */
    /**
     * @brief Get the number of CRC failures recorded in @p ctx.
     *
     * @param ctx Context to query (may be NULL).
     * @return Number of CRC failures (0 if ctx is NULL).
     */
    uint32_t crumbs_get_crc_error_count(const crumbs_context_t *ctx);

    /**
     * @brief Query whether the last decoded frame had a valid CRC.
     *
     * @param ctx Context to query (may be NULL).
     * @return 1 if last decode had valid CRC, 0 otherwise (or if ctx==NULL).
     */
    int crumbs_last_crc_ok(const crumbs_context_t *ctx);

    /**
     * @brief Reset CRC statistics tracked in the context.
     *
     * Clears the error count and marks last CRC as OK.
     */
    void crumbs_reset_crc_stats(crumbs_context_t *ctx);
    /** @} */

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_H */
