#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "crumbs.h"
#include "crumbs_linux.h"
#include "crumbs_message_helpers.h"

#define MAX_FOUND 16u
#define BUS_TIMEOUT_US 25000u
#define QUERY_DELAY_US 10000u
#define EZO_WAIT_MS 1000u
#define EZO_FRAME_LEN 31u

static const uint8_t kCrumbsCandidates[] = {0x0A, 0x14, 0x15};
static const uint8_t kBoschCandidates[] = {0x76, 0x77};

static void print_hex_bytes(const uint8_t *data, size_t len)
{
    size_t i = 0u;
    for (i = 0u; i < len; ++i)
    {
        printf("%s0x%02X", (i == 0u) ? "" : " ", data[i]);
    }
}

static const char *ezo_status_name(uint8_t status)
{
    switch (status)
    {
    case 1:
        return "SUCCESS";
    case 2:
        return "FAIL";
    case 254:
        return "NOT_READY";
    case 255:
        return "NO_DATA";
    default:
        return "UNKNOWN";
    }
}

static void extract_ezo_text(const uint8_t *frame, size_t frame_len, char *out, size_t out_len)
{
    size_t i = 0u;
    size_t j = 0u;

    if (!frame || frame_len == 0u || !out || out_len == 0u)
    {
        return;
    }

    out[0] = '\0';

    for (i = 1u; i < frame_len && j + 1u < out_len; ++i)
    {
        uint8_t b = frame[i];
        if (b == 0u)
        {
            break;
        }

        if (b >= 32u && b <= 126u)
        {
            out[j++] = (char)b;
        }
        else
        {
            out[j++] = '.';
        }
    }

    while (j > 0u && out[j - 1u] == ' ')
    {
        --j;
    }

    out[j] = '\0';
}

static bool query_default_reply(crumbs_context_t *ctx, crumbs_linux_i2c_t *bus, uint8_t addr)
{
    crumbs_message_t q;
    crumbs_message_t reply;
    int rc;

    crumbs_msg_init(&q, 0u, CRUMBS_CMD_SET_REPLY);
    if (crumbs_msg_add_u8(&q, 0u) != 0)
    {
        printf("CRUMBS addr=0x%02X query build failed\n", addr);
        return false;
    }

    rc = crumbs_controller_send(ctx, addr, &q, crumbs_linux_i2c_write, (void *)bus);
    if (rc != 0)
    {
        printf("CRUMBS addr=0x%02X query send failed rc=%d\n", addr, rc);
        return false;
    }

    crumbs_linux_delay_us(QUERY_DELAY_US);

    rc = crumbs_controller_read(ctx, addr, &reply, crumbs_linux_read, (void *)bus);
    if (rc != 0)
    {
        printf("CRUMBS addr=0x%02X query read failed rc=%d\n", addr, rc);
        return false;
    }

    printf("CRUMBS addr=0x%02X type=0x%02X reply_op=0x%02X len=%u data=",
           addr, reply.type_id, reply.opcode, reply.data_len);
    print_hex_bytes(reply.data, reply.data_len);

    if (reply.data_len >= 5u)
    {
        uint16_t crumbs_ver = 0u;
        uint8_t module_maj = 0u;
        uint8_t module_min = 0u;
        uint8_t module_pat = 0u;

        if (crumbs_msg_read_u16(reply.data, reply.data_len, 0u, &crumbs_ver) == 0 &&
            crumbs_msg_read_u8(reply.data, reply.data_len, 2u, &module_maj) == 0 &&
            crumbs_msg_read_u8(reply.data, reply.data_len, 3u, &module_min) == 0 &&
            crumbs_msg_read_u8(reply.data, reply.data_len, 4u, &module_pat) == 0)
        {
            printf(" crumbs_ver=0x%04X module=%u.%u.%u", crumbs_ver, module_maj, module_min, module_pat);
        }
    }

    printf("\n");
    return true;
}

static bool test_ezo_sensor(crumbs_context_t *ctx, crumbs_linux_i2c_t *bus, const char *name, uint8_t addr)
{
    (void)ctx;

    crumbs_device_t dev;
    const uint8_t cmd[] = {'R'};
    uint8_t frame[EZO_FRAME_LEN];
    char text[96];
    int rc;
    int n;
    uint8_t status;

    memset(&dev, 0, sizeof(dev));
    dev.ctx = ctx;
    dev.addr = addr;
    dev.write_fn = crumbs_linux_i2c_write;
    dev.read_fn = crumbs_linux_read;
    dev.delay_fn = crumbs_linux_delay_us;
    dev.io = (void *)bus;

    rc = crumbs_i2c_dev_write(&dev, cmd, sizeof(cmd));
    if (rc != CRUMBS_I2C_DEV_OK)
    {
        printf("EZO %s addr=0x%02X write failed rc=%d\n", name, addr, rc);
        return false;
    }

    crumbs_linux_delay_us((uint32_t)(EZO_WAIT_MS * 1000u));

    n = dev.read_fn(dev.io, dev.addr, frame, sizeof(frame), BUS_TIMEOUT_US);
    if (n <= 0)
    {
        printf("EZO %s addr=0x%02X read failed rc=%d\n", name, addr, n);
        return false;
    }

    status = frame[0];
    extract_ezo_text(frame, (size_t)n, text, sizeof(text));

    printf("EZO %s addr=0x%02X status=0x%02X(%s) text=\"%s\"\n",
           name, addr, status, ezo_status_name(status), text);

    return status == 1u;
}

static bool test_bosch_candidate(crumbs_context_t *ctx,
                                 crumbs_linux_i2c_t *bus,
                                 uint8_t addr,
                                 bool *is_bosch_out)
{
    crumbs_device_t dev;
    uint8_t chip_id = 0u;
    uint8_t raw[6];
    int rc_chip;
    int rc_raw;
    const char *model = "UNKNOWN";

    if (!is_bosch_out)
    {
        return false;
    }

    *is_bosch_out = false;

    memset(&dev, 0, sizeof(dev));
    dev.ctx = ctx;
    dev.addr = addr;
    dev.write_fn = crumbs_linux_i2c_write;
    dev.read_fn = crumbs_linux_read;
    dev.delay_fn = crumbs_linux_delay_us;
    dev.io = (void *)bus;

    rc_chip = crumbs_i2c_dev_read_reg_u8(&dev,
                                         0xD0u,
                                         &chip_id,
                                         1u,
                                         BUS_TIMEOUT_US,
                                         1,
                                         crumbs_linux_write_then_read);

    if (rc_chip != CRUMBS_I2C_DEV_OK)
    {
        printf("Bosch addr=0x%02X chip_id read failed rc=%d\n", addr, rc_chip);
        return false;
    }

    if (chip_id == 0x58u)
    {
        model = "BMP280";
        *is_bosch_out = true;
    }
    else if (chip_id == 0x60u)
    {
        model = "BME280";
        *is_bosch_out = true;
    }

    memset(raw, 0, sizeof(raw));
    rc_raw = crumbs_i2c_dev_read_reg_u8(&dev,
                                        0xF7u,
                                        raw,
                                        sizeof(raw),
                                        BUS_TIMEOUT_US,
                                        1,
                                        crumbs_linux_write_then_read);

    printf("Bosch addr=0x%02X chip_id=0x%02X model=%s raw_rc=%d raw=",
           addr, chip_id, model, rc_raw);
    print_hex_bytes(raw, sizeof(raw));
    printf("\n");

    return rc_raw == CRUMBS_I2C_DEV_OK;
}

int main(int argc, char **argv)
{
    const char *device_path = "/dev/i2c-1";
    crumbs_context_t ctx;
    crumbs_linux_i2c_t bus;
    uint8_t found[MAX_FOUND];
    uint8_t types[MAX_FOUND];
    bool overall_ok = true;
    int rc;
    int n;
    size_t i;

    if (argc > 2)
    {
        fprintf(stderr, "Usage: %s [i2c-dev]\n", argv[0]);
        return 2;
    }

    if (argc == 2)
    {
        device_path = argv[1];
    }

    printf("CRUMBS mixed-bus lab validation\n");
    printf("Bus: %s\n", device_path);
    printf("Expected CRUMBS addrs: 0x0A, 0x14, 0x15\n");
    printf("Expected EZO addrs: pH=0x63, DO=0x61\n");
    printf("Optional Bosch addrs: 0x76 or 0x77\n\n");

    rc = crumbs_linux_init_controller(&ctx, &bus, device_path, BUS_TIMEOUT_US);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR: crumbs_linux_init_controller failed (%d)\n", rc);
        return 1;
    }

    memset(found, 0, sizeof(found));
    memset(types, 0, sizeof(types));
    n = crumbs_controller_scan_for_crumbs_candidates(&ctx,
                                                     kCrumbsCandidates,
                                                     sizeof(kCrumbsCandidates) / sizeof(kCrumbsCandidates[0]),
                                                     1,
                                                     crumbs_linux_i2c_write,
                                                     crumbs_linux_read,
                                                     (void *)&bus,
                                                     found,
                                                     types,
                                                     MAX_FOUND,
                                                     BUS_TIMEOUT_US);
    if (n < 0)
    {
        fprintf(stderr, "ERROR: CRUMBS scan failed (%d)\n", n);
        crumbs_linux_close(&bus);
        return 1;
    }

    printf("CRUMBS scan result: %d\n", n);
    for (i = 0u; i < (size_t)n; ++i)
    {
        printf("  addr=0x%02X type=0x%02X\n", found[i], types[i]);
    }

    /*
     * Validation is based on direct query/reply at expected addresses.
     * Scan output is informational only because some devices may not
     * participate in strict read-only scan behavior.
     */
    for (i = 0u; i < (sizeof(kCrumbsCandidates) / sizeof(kCrumbsCandidates[0])); ++i)
    {
        if (!query_default_reply(&ctx, &bus, kCrumbsCandidates[i]))
        {
            printf("CRUMBS validation failed at expected addr=0x%02X\n", kCrumbsCandidates[i]);
            overall_ok = false;
        }
    }

    if (!test_ezo_sensor(&ctx, &bus, "pH", 0x63u))
    {
        overall_ok = false;
    }

    if (!test_ezo_sensor(&ctx, &bus, "DO", 0x61u))
    {
        overall_ok = false;
    }

    {
        bool bosch_any = false;
        for (i = 0u; i < (sizeof(kBoschCandidates) / sizeof(kBoschCandidates[0])); ++i)
        {
            bool is_bosch = false;
            bool ok = test_bosch_candidate(&ctx, &bus, kBoschCandidates[i], &is_bosch);
            if (ok && is_bosch)
            {
                bosch_any = true;
            }
        }

        if (!bosch_any)
        {
            printf("Bosch sensor optional check skipped (no BMP/BME280 detected at 0x76/0x77)\n");
        }
    }

    crumbs_linux_close(&bus);

    printf("\nRESULT: %s\n", overall_ok ? "PASS" : "FAIL");
    return overall_ok ? 0 : 1;
}
