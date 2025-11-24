#ifndef CRUMBS_ARDUINO_H
#define CRUMBS_ARDUINO_H

#include <stdint.h>
#include <stddef.h>

#include "crumbs.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize a CRUMBS context for use as an I2C controller on Arduino.
     *
     * This uses the default global Wire instance and calls Wire.begin().
     * You can still call crumbs_set_callbacks() afterwards if you want to
     * attach any controller-side metadata or debug hooks.
     */
    void crumbs_arduino_init_controller(crumbs_context_t *ctx);

    /**
     * @brief Initialize a CRUMBS context for use as an I2C peripheral on Arduino.
     *
     * This:
     *   - calls crumbs_init(ctx, CRUMBS_ROLE_PERIPHERAL, address);
     *   - calls Wire.begin(address);
     *   - registers internal Wire.onReceive/onRequest handlers that forward
     *     into the CRUMBS C core.
     *
     * You must call crumbs_set_callbacks() to install your on_message/on_request
     * callbacks before or after this, as long as it is done before traffic arrives.
     */
    void crumbs_arduino_init_peripheral(crumbs_context_t *ctx, uint8_t address);

    /**
     * @brief Arduino implementation of crumbs_i2c_write_fn using Wire.
     *
     * This is intended to be passed to crumbs_controller_send().
     *
     * The @p user_ctx parameter is expected to be:
     *   - a pointer to a TwoWire instance (e.g., &Wire), or
     *   - NULL, in which case the default &Wire is used.
     *
     * Return value:
     *   - 0 on success (Wire.endTransmission() == 0 and all bytes written)
     *   - non-zero on error.
     */
    int crumbs_arduino_wire_write(void *user_ctx,
                                  uint8_t addr,
                                  const uint8_t *data,
                                  size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRUMBS_ARDUINO_H */
