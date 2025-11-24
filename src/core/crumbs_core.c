#include "crumbs_core.h"

#include <string.h> /* memcpy, memset */

/* ---- Internal constants & helpers (file-local) ------------------------- */

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(float) == 4, "CRUMBS requires 32-bit IEEE 754 float");
#endif

/* Header = type_id + command_type */
static const size_t k_header_len = 2u;
static const size_t k_data_len = CRUMBS_DATA_LENGTH;
static const size_t k_payload_len = 2u + (CRUMBS_DATA_LENGTH * sizeof(float));
static const size_t k_crc_len = 1u;
static const size_t k_frame_len = 2u + (CRUMBS_DATA_LENGTH * sizeof(float)) + 1u;

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(CRUMBS_MESSAGE_SIZE == 31u, "CRUMBS_MESSAGE_SIZE must be 31");
_Static_assert(CRUMBS_MESSAGE_SIZE == k_frame_len, "CRUMBS_MESSAGE_SIZE must equal serialized frame length");
#endif

static void write_float_le(float value, uint8_t *out)
{
    uint32_t raw = 0;
    memcpy(&raw, &value, sizeof(raw));
    out[0] = (uint8_t)(raw & 0xFFu);
    out[1] = (uint8_t)((raw >> 8) & 0xFFu);
    out[2] = (uint8_t)((raw >> 16) & 0xFFu);
    out[3] = (uint8_t)((raw >> 24) & 0xFFu);
}

static float read_float_le(const uint8_t *in)
{
    uint32_t raw =
        ((uint32_t)in[0]) |
        ((uint32_t)in[1] << 8) |
        ((uint32_t)in[2] << 16) |
        ((uint32_t)in[3] << 24);
    float value = 0.0f;
    memcpy(&value, &raw, sizeof(value));
    return value;
}

/* ---- Public API implementation ---------------------------------------- */

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

size_t crumbs_encode_message(const crumbs_message_t *msg,
                             uint8_t *buffer,
                             size_t buffer_len)
{
    if (!msg || !buffer || buffer_len < k_frame_len)
    {
        return 0u;
    }

    size_t index = 0u;

    buffer[index++] = msg->type_id;
    buffer[index++] = msg->command_type;

    for (size_t i = 0; i < k_data_len; ++i)
    {
        write_float_le(msg->data[i], &buffer[index]);
        index += sizeof(float);
    }

    /* Compute CRC over payload (header + data) */
    crumbs_crc8_t crc = crumbs_crc8(buffer, k_payload_len);
    buffer[index++] = crc;

    /* Preserve CRC in struct for caller's convenience. */
    /* Note: slice_address is not serialized. */
    /* Caller can set msg->crc8 before or after if desired. */
    (void)msg; /* struct not modified here */

    return index; /* should be k_frame_len */
}

int crumbs_decode_message(const uint8_t *buffer,
                          size_t buffer_len,
                          crumbs_message_t *msg,
                          crumbs_context_t *ctx)
{
    if (!buffer || !msg)
    {
        return -1;
    }

    if (buffer_len < k_frame_len)
    {
        if (ctx)
        {
            ctx->last_crc_ok = 0u;
        }
        return -1; /* too small */
    }

    const crumbs_crc8_t computed = crumbs_crc8(buffer, k_payload_len);
    const crumbs_crc8_t received = buffer[k_payload_len];

    if (computed != received)
    {
        if (ctx)
        {
            ctx->last_crc_ok = 0u;
            ctx->crc_error_count++;
        }
        return -2; /* CRC mismatch */
    }

    size_t index = 0u;

    /* Note: slice_address is not transmitted; caller decides how to route it. */
    msg->type_id = buffer[index++];
    msg->command_type = buffer[index++];

    for (size_t i = 0; i < k_data_len; ++i)
    {
        msg->data[i] = read_float_le(&buffer[index]);
        index += sizeof(float);
    }

    msg->crc8 = received;

    if (ctx)
    {
        ctx->last_crc_ok = 1u;
    }

    return 0;
}

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

    uint8_t frame[CRUMBS_MESSAGE_SIZE];
    size_t written = crumbs_encode_message(msg, frame, sizeof(frame));
    if (written == 0u)
    {
        return -3;
    }

    return write_fn(write_ctx, target_addr, frame, written);
}

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

    if (ctx->on_message)
    {
        ctx->on_message(ctx, &msg);
    }

    return 0;
}

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

/* ---- CRC stats helpers ------------------------------------------------- */

uint32_t crumbs_get_crc_error_count(const crumbs_context_t *ctx)
{
    return ctx ? ctx->crc_error_count : 0u;
}

int crumbs_last_crc_ok(const crumbs_context_t *ctx)
{
    return (ctx && ctx->last_crc_ok) ? 1 : 0;
}

void crumbs_reset_crc_stats(crumbs_context_t *ctx)
{
    if (!ctx)
    {
        return;
    }
    ctx->crc_error_count = 0u;
    ctx->last_crc_ok = 1u;
}
