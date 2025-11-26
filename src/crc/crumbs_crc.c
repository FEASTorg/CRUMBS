/**
 * @file
 * @brief Small wrapper around the generated CRC implementation.
 */

#include "crumbs_crc.h"
#include "crc8_nibble.h"

/**
 * @brief Compute CRC-8 value using the generated table-driven implementation.
 *
 * @param data Input buffer (may be NULL only when len == 0).
 * @param len Length of buffer in bytes.
 * @return CRC-8 value of the input bytes.
 */
crumbs_crc8_t crumbs_crc8(const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        return 0u;
    }

    crc_t crc = crc_init();
    crc = crc_update(crc, data, len);
    crc = crc_finalize(crc);

    return (crumbs_crc8_t)crc;
}
