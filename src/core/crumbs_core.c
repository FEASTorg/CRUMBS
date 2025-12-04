/**
 * @file
 * @brief Core CRUMBS implementation (encode/decode and controller/peripheral helpers).
 */

#include "crumbs.h"

#include <string.h> /* memcpy, memset */

/* ---- Internal constants (file-local) ----------------------------------- */

/** @brief Minimum frame size: type_id + command_type + data_len + crc8 = 4 bytes. */
static const size_t k_min_frame_len = 4u;

/** @brief Header size: type_id + command_type + data_len = 3 bytes. */
static const size_t k_header_len = 3u;

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(CRUMBS_MESSAGE_MAX_SIZE == 31u, "CRUMBS_MESSAGE_MAX_SIZE must be 31");
_Static_assert(CRUMBS_MAX_PAYLOAD == 27u, "CRUMBS_MAX_PAYLOAD must be 27");
#endif

/* ---- Public API implementation ---------------------------------------- */

/**
 * @brief Initialize a CRUMBS context structure.
 *
 * Clears the structure and sets the role and address as provided.
 */
void crumbs_init(crumbs_context_t *ctx,
                 crumbs_role_t role,
                 uint8_t address)
{
    if (!ctx)
    {
        return;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->role = role;
    ctx->address = (role == CRUMBS_ROLE_PERIPHERAL) ? address : 0u;
}

/**
 * @brief Install callbacks and user data on an existing context.
 */
void crumbs_set_callbacks(crumbs_context_t *ctx,
                          crumbs_message_cb_t on_message,
                          crumbs_request_cb_t on_request,
                          void *user_data)
{
    if (!ctx)
    {
        return;
    }

    ctx->on_message = on_message;
    ctx->on_request = on_request;
    ctx->user_data = user_data;
}

/**
 * @brief Register a handler for a specific command type.
 */
int crumbs_register_handler(crumbs_context_t *ctx,
                            uint8_t command_type,
                            crumbs_handler_fn fn,
                            void *user_data)
{
    if (!ctx)
    {
        return -1;
    }

    ctx->handlers[command_type] = fn;
    ctx->handler_userdata[command_type] = user_data;
    return 0;
}

/**
 * @brief Unregister a handler for a specific command type.
 */
int crumbs_unregister_handler(crumbs_context_t *ctx,
                              uint8_t command_type)
{
    return crumbs_register_handler(ctx, command_type, NULL, NULL);
}

/**
 * @brief Serialize a crumbs_message_t into a flat byte buffer.
 *
 * Wire format: [type_id, command_type, data_len, data[0..data_len-1], crc8]
 * Returns the encoded length (4 + data_len) on success, or 0 on error.
 */
size_t crumbs_encode_message(const crumbs_message_t *msg,
                             uint8_t *buffer,
                             size_t buffer_len)
{
    if (!msg || !buffer)
    {
        return 0u;
    }

    /* Validate data_len is within bounds. */
    if (msg->data_len > CRUMBS_MAX_PAYLOAD)
    {
        return 0u;
    }

    /* Compute required frame size: header (3) + data_len + crc (1). */
    size_t frame_len = k_header_len + msg->data_len + 1u;
    if (buffer_len < frame_len)
    {
        return 0u;
    }

    size_t index = 0u;

    buffer[index++] = msg->type_id;
    buffer[index++] = msg->command_type;
    buffer[index++] = msg->data_len;

    /* Copy payload bytes. */
    if (msg->data_len > 0u)
    {
        memcpy(&buffer[index], msg->data, msg->data_len);
        index += msg->data_len;
    }

    /* Compute CRC over header + payload (bytes 0 through index-1). */
    crumbs_crc8_t crc = crumbs_crc8(buffer, index);
    buffer[index++] = crc;

    return index; /* 4 + data_len */
}

/**
 * @brief Decode a serialized frame into a crumbs_message_t.
 *
 * Validates frame structure and CRC. Updates CRC state in @p ctx if provided.
 */
int crumbs_decode_message(const uint8_t *buffer,
                          size_t buffer_len,
                          crumbs_message_t *msg,
                          crumbs_context_t *ctx)
{
    if (!buffer || !msg)
    {
        return -1;
    }

    /* Minimum frame: type_id + command_type + data_len + crc8 = 4 bytes. */
    if (buffer_len < k_min_frame_len)
    {
        if (ctx)
        {
            ctx->last_crc_ok = 0u;
        }
        return -1; /* too small */
    }

    /* Extract data_len from header. */
    uint8_t data_len = buffer[2];
    if (data_len > CRUMBS_MAX_PAYLOAD)
    {
        if (ctx)
        {
            ctx->last_crc_ok = 0u;
        }
        return -1; /* invalid data_len */
    }

    /* Expected frame length: header (3) + data_len + crc (1). */
    size_t expected_len = k_header_len + data_len + 1u;
    if (buffer_len < expected_len)
    {
        if (ctx)
        {
            ctx->last_crc_ok = 0u;
        }
        return -1; /* truncated frame */
    }

    /* CRC is computed over header + payload (bytes 0 through header+data_len-1). */
    size_t crc_span = k_header_len + data_len;
    const crumbs_crc8_t computed = crumbs_crc8(buffer, crc_span);
    const crumbs_crc8_t received = buffer[crc_span];

    if (computed != received)
    {
        if (ctx)
        {
            ctx->last_crc_ok = 0u;
            ctx->crc_error_count++;
        }
        return -2; /* CRC mismatch */
    }

    /* Populate message fields. slice_address is not transmitted. */
    msg->type_id = buffer[0];
    msg->command_type = buffer[1];
    msg->data_len = data_len;

    if (data_len > 0u)
    {
        memcpy(msg->data, &buffer[k_header_len], data_len);
    }

    msg->crc8 = received;

    if (ctx)
    {
        ctx->last_crc_ok = 1u;
    }

    return 0;
}

/**
 * @brief Helper used by controllers to send a CRUMBS frame.
 */
int crumbs_controller_send(const crumbs_context_t *ctx,
                           uint8_t target_addr,
                           const crumbs_message_t *msg,
                           crumbs_i2c_write_fn write_fn,
                           void *write_ctx)
{
    if (!ctx || !msg || !write_fn)
    {
        return -1;
    }

    if (ctx->role != CRUMBS_ROLE_CONTROLLER)
    {
        return -2;
    }

    uint8_t frame[CRUMBS_MESSAGE_MAX_SIZE];
    size_t written = crumbs_encode_message(msg, frame, sizeof(frame));
    if (written == 0u)
    {
        return -3;
    }

    return write_fn(write_ctx, target_addr, frame, written);
}

/**
 * @brief Peripheral-side handler for raw bytes received by a HAL.
 */
int crumbs_peripheral_handle_receive(crumbs_context_t *ctx,
                                     const uint8_t *buffer,
                                     size_t len)
{
    if (!ctx || ctx->role != CRUMBS_ROLE_PERIPHERAL || !buffer)
    {
        return -1;
    }

    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));

    int rc = crumbs_decode_message(buffer, len, &msg, ctx);
    if (rc != 0)
    {
        return rc;
    }

    /*
     * slice_address is not encoded on the wire. If you want to track "which
     * slice" a message belongs to, you can set msg.slice_address here from
     * ctx->address or some higher-level routing logic.
     */
    msg.slice_address = ctx->address;

    /* Invoke general on_message callback if set. */
    if (ctx->on_message)
    {
        ctx->on_message(ctx, &msg);
    }

    /* Dispatch to per-command handler if registered. */
    crumbs_handler_fn handler = ctx->handlers[msg.command_type];
    if (handler)
    {
        handler(ctx,
                msg.command_type,
                msg.data,
                msg.data_len,
                ctx->handler_userdata[msg.command_type]);
    }

    return 0;
}

/**
 * @brief Build an encoded reply using the configured on_request callback.
 */
int crumbs_peripheral_build_reply(crumbs_context_t *ctx,
                                  uint8_t *out_buf,
                                  size_t out_buf_len,
                                  size_t *out_len)
{
    if (out_len)
    {
        *out_len = 0u;
    }

    if (!ctx || ctx->role != CRUMBS_ROLE_PERIPHERAL || !out_buf)
    {
        return -1;
    }

    if (!ctx->on_request)
    {
        /* No reply configured. */
        return 0;
    }

    crumbs_message_t msg;
    memset(&msg, 0, sizeof(msg));

    /* Allow application to fill in the reply. */
    ctx->on_request(ctx, &msg);

    size_t written = crumbs_encode_message(&msg, out_buf, out_buf_len);
    if (written == 0u)
    {
        return -2;
    }

    if (out_len)
    {
        *out_len = written;
    }
    return 0;
}

/**
 * @brief Probe an address range looking for CRUMBS-capable devices.
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
                                      uint32_t timeout_us)
{
    if (!ctx || !read_fn || !found || max_found == 0u)
        return -1; /* invalid args */

    if (start_addr > end_addr)
        return -1;

    uint8_t buf[CRUMBS_MESSAGE_MAX_SIZE];
    size_t count = 0u;

    for (int addr = start_addr; addr <= end_addr; ++addr)
    {
        /* Attempt a direct read first (many peripherals reply on request).
           read_fn returns number of bytes read on success, negative on error. */
        int n = read_fn(io_ctx, (uint8_t)addr, buf, (size_t)CRUMBS_MESSAGE_MAX_SIZE, timeout_us);
        if (n >= (int)k_min_frame_len)
        {
            crumbs_message_t m;
            if (crumbs_decode_message(buf, (size_t)n, &m, NULL) == 0)
            {
                if (count < max_found)
                    found[count] = (uint8_t)addr;
                ++count;
                if (count >= max_found)
                    break;
                continue; /* next address */
            }
        }

        /* In non-strict mode we try to stimulate a reply by sending a small
           CRUMBS frame then attempting to read again. This helps with
           peripherals that only respond after being written to. */
        if (!strict && write_fn)
        {
            crumbs_message_t probe;
            memset(&probe, 0, sizeof(probe));

            /* Send a probe message; ignore any error from send. */
            (void)crumbs_controller_send(ctx, (uint8_t)addr, &probe, write_fn, io_ctx);

            int n2 = read_fn(io_ctx, (uint8_t)addr, buf, (size_t)CRUMBS_MESSAGE_MAX_SIZE, timeout_us);
            if (n2 >= (int)k_min_frame_len)
            {
                crumbs_message_t m2;
                if (crumbs_decode_message(buf, (size_t)n2, &m2, NULL) == 0)
                {
                    if (count < max_found)
                        found[count] = (uint8_t)addr;
                    ++count;
                    if (count >= max_found)
                        break;
                }
            }
        }
    }

    return (int)count;
}

/* ---- CRC stats helpers ------------------------------------------------- */

/**
 * @brief Get the number of CRC errors observed by the context.
 *
 * @param ctx Context to query (may be NULL).
 * @return Count of CRC failures recorded by @p ctx (0 if ctx is NULL).
 */
uint32_t crumbs_get_crc_error_count(const crumbs_context_t *ctx)
{
    return ctx ? ctx->crc_error_count : 0u;
}

/**
 * @brief Return 1 if the last decoded frame had a valid CRC.
 *
 * @param ctx Context to query (may be NULL).
 * @return 1 if the last decode was CRC-ok, 0 otherwise.
 */
int crumbs_last_crc_ok(const crumbs_context_t *ctx)
{
    return (ctx && ctx->last_crc_ok) ? 1 : 0;
}

/**
 * @brief Reset CRC statistics for @p ctx.
 *
 * This clears the error count and sets last_crc_ok to true.
 */
void crumbs_reset_crc_stats(crumbs_context_t *ctx)
{
    if (!ctx)
    {
        return;
    }
    ctx->crc_error_count = 0u;
    ctx->last_crc_ok = 1u;
}
