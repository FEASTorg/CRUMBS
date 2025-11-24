#include <Arduino.h>
#include <Wire.h>

#include "crumbs_arduino.h"
#include "crumbs_message.h"

#ifndef CRUMBS_DEFAULT_TWI_FREQ
#define CRUMBS_DEFAULT_TWI_FREQ 100000UL
#endif

// Single global CRUMBS context pointer used by the Wire callbacks.
// This matches Arduino's single-bus, single-instance pattern.
// If you ever need multiple instances, this is where we would extend.
static crumbs_context_t *g_crumbs_ctx = nullptr;

/* ------------------------------------------------------------------------- */
/* Internal helpers                                                          */
/* ------------------------------------------------------------------------- */

static void crumbs_arduino_on_receive(int numBytes)
{
    if (g_crumbs_ctx == nullptr || numBytes <= 0)
    {
        // Drain any pending bytes to avoid clogging the Wire buffer.
        while (Wire.available() > 0)
        {
            (void)Wire.read();
        }
        return;
    }

    uint8_t buffer[CRUMBS_MESSAGE_SIZE];
    size_t index = 0;

    while (Wire.available() > 0 && index < sizeof(buffer))
    {
        buffer[index++] = static_cast<uint8_t>(Wire.read());
    }

    // If there were more bytes than we can store, drain them.
    while (Wire.available() > 0)
    {
        (void)Wire.read();
    }

    // Let the core decode and invoke on_message() if valid.
    (void)crumbs_peripheral_handle_receive(g_crumbs_ctx, buffer, index);
}

static void crumbs_arduino_on_request()
{
    if (g_crumbs_ctx == nullptr)
    {
        return;
    }

    uint8_t frame[CRUMBS_MESSAGE_SIZE];
    size_t frame_len = 0;

    int rc = crumbs_peripheral_build_reply(g_crumbs_ctx,
                                           frame,
                                           sizeof(frame),
                                           &frame_len);
    if (rc == 0 && frame_len > 0)
    {
        Wire.write(frame, frame_len);
    }
}

/* ------------------------------------------------------------------------- */
/* Public API                                                                */
/* ------------------------------------------------------------------------- */

extern "C" void crumbs_arduino_init_controller(crumbs_context_t *ctx)
{
    if (ctx == nullptr)
    {
        return;
    }

    // Initialize CRUMBS context as controller.
    crumbs_init(ctx, CRUMBS_ROLE_CONTROLLER, 0);

    // Use the default Wire bus.
    Wire.begin();
#if defined(TWI_FREQ) || defined(TWBR)
    // If the platform supports setClock(), use a standard frequency.
    Wire.setClock(CRUMBS_DEFAULT_TWI_FREQ);
#endif

    // Controller mode does not use Wire callbacks; user will call
    // crumbs_controller_send() with crumbs_arduino_wire_write().
    g_crumbs_ctx = ctx;
}

extern "C" void crumbs_arduino_init_peripheral(crumbs_context_t *ctx, uint8_t address)
{
    if (ctx == nullptr)
    {
        return;
    }

    // Initialize CRUMBS context as peripheral.
    crumbs_init(ctx, CRUMBS_ROLE_PERIPHERAL, address);

    // Configure Wire as I2C slave at the given address.
    Wire.begin(address);
#if defined(TWI_FREQ) || defined(TWBR)
    Wire.setClock(CRUMBS_DEFAULT_TWI_FREQ);
#endif

    g_crumbs_ctx = ctx;

    // Register callbacks that route into the CRUMBS core.
    Wire.onReceive(crumbs_arduino_on_receive);
    Wire.onRequest(crumbs_arduino_on_request);
}

extern "C" int crumbs_arduino_wire_write(void *user_ctx,
                                         uint8_t addr,
                                         const uint8_t *data,
                                         size_t len)
{
    // user_ctx is expected to be a TwoWire*; default to &Wire if null.
    TwoWire *wire = &Wire;
    if (user_ctx != nullptr)
    {
        wire = static_cast<TwoWire *>(user_ctx);
    }

    if (data == nullptr && len > 0)
    {
        return -1;
    }

    wire->beginTransmission(addr);
    size_t written = 0;
    if (len > 0)
    {
        written = wire->write(data, len);
    }
    uint8_t err = wire->endTransmission(); // 0 means success

    if (err != 0)
    {
        return (int)err; // propagate Wire error code
    }
    if (written != len)
    {
        return -2; // partial write
    }
    return 0;
}
