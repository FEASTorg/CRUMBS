#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "crumbs.h"

int main(void)
{
    crumbs_context_t ctx;
    crumbs_init(&ctx, CRUMBS_ROLE_CONTROLLER, 0);

    crumbs_message_t m;
    memset(&m, 0, sizeof(m));
    m.type_id = 0x10;
    m.command_type = 0x01;
    m.data[0] = 3.1415926f;

    uint8_t buf[CRUMBS_MESSAGE_SIZE];
    size_t w = crumbs_encode_message(&m, buf, sizeof(buf));
    if (w != CRUMBS_MESSAGE_SIZE)
    {
        fprintf(stderr, "encode failed (w=%zu)\n", w);
        return 2;
    }

    printf("Encoded %zu bytes:\n", w);
    for (size_t i = 0; i < w; ++i)
        printf("%02X ", buf[i]);
    printf("\n");

    crumbs_message_t out;
    int rc = crumbs_decode_message(buf, w, &out, &ctx);
    if (rc != 0)
    {
        fprintf(stderr, "decode failed (rc=%d)\n", rc);
        return 3;
    }

    printf("Decoded message: type_id=%u cmd=%u data[0]=%f crc_ok=%d\n",
           out.type_id, out.command_type, out.data[0], crumbs_last_crc_ok(&ctx));

    return 0;
}
