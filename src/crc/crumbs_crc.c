#include "crumbs_crc.h"
#include "crc8_nibble.h"

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
