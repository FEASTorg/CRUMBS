#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"
#include "crumbs_linux.h"

#define MAX_CANDIDATES 32u
#define MAX_SENSOR_IO 64u

static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage:\n"
            "  %s <i2c-dev> scan <candidate_csv> [strict|non-strict]\n"
            "  %s <i2c-dev> read-u8 <sensor_addr> <reg_u8> <len> [repeat|norepeat]\n"
            "  %s <i2c-dev> read-u16 <sensor_addr> <reg_u16> <len> [repeat|norepeat]\n"
            "  %s <i2c-dev> read-ex <sensor_addr> <reg_csv> <len> [repeat|norepeat]\n"
            "  %s <i2c-dev> write-u8 <sensor_addr> <reg_u8> <data_csv>\n"
            "  %s <i2c-dev> write-u16 <sensor_addr> <reg_u16> <data_csv>\n"
            "  %s <i2c-dev> write-ex <sensor_addr> <reg_csv> <data_csv>\n"
            "\n"
            "Examples:\n"
            "  %s /dev/i2c-1 scan 0x20,0x21,0x30 strict\n"
            "  %s /dev/i2c-1 read-u8 0x76 0xD0 1 repeat  # BMP/BME280 chip-id\n"
            "  %s /dev/i2c-1 read-u16 0x40 0x0100 6 repeat\n"
            "  %s /dev/i2c-1 read-ex 0x1E 0x20,0x08 6 repeat\n"
            "  %s /dev/i2c-1 write-u8 0x76 0xF4 0x27\n",
            prog, prog, prog, prog, prog, prog, prog, prog, prog, prog, prog, prog);
}

static int parse_u32(const char *s, uint32_t *out)
{
    if (!s || !out)
        return -1;
    char *end = NULL;
    unsigned long v = strtoul(s, &end, 0);
    if (end == s || *end != '\0')
        return -1;
    *out = (uint32_t)v;
    return 0;
}

static int parse_i2c_addr(const char *s, uint8_t *out)
{
    uint32_t v = 0;
    if (parse_u32(s, &v) != 0 || v > 0x7Fu)
        return -1;
    *out = (uint8_t)v;
    return 0;
}

static int parse_u8_byte(const char *s, uint8_t *out)
{
    uint32_t v = 0;
    if (parse_u32(s, &v) != 0 || v > 0xFFu)
        return -1;
    *out = (uint8_t)v;
    return 0;
}

static int parse_u16(const char *s, uint16_t *out)
{
    uint32_t v = 0;
    if (parse_u32(s, &v) != 0 || v > 0xFFFFu)
        return -1;
    *out = (uint16_t)v;
    return 0;
}

static int parse_repeat_mode(const char *s, int *require_repeated_start)
{
    if (!require_repeated_start)
        return -1;
    if (!s)
    {
        *require_repeated_start = 1;
        return 0;
    }
    if (strcmp(s, "repeat") == 0 || strcmp(s, "rs") == 0 || strcmp(s, "1") == 0)
    {
        *require_repeated_start = 1;
        return 0;
    }
    if (strcmp(s, "norepeat") == 0 || strcmp(s, "0") == 0)
    {
        *require_repeated_start = 0;
        return 0;
    }
    return -1;
}

static int parse_u8_csv(const char *csv, uint8_t *out, size_t max_out, size_t *out_count)
{
    if (!csv || !out || !out_count || max_out == 0u)
        return -1;

    char temp[512];
    size_t n = strlen(csv);
    if (n == 0u || n >= sizeof(temp))
        return -1;
    memcpy(temp, csv, n + 1u);

    size_t count = 0u;
    char *cursor = temp;
    while (*cursor != '\0')
    {
        char *tok = cursor;
        char *comma = strchr(cursor, ',');
        if (comma)
        {
            *comma = '\0';
            if (*(comma + 1) == '\0')
                return -1;
            cursor = comma + 1;
        }
        else
        {
            cursor += strlen(cursor);
        }

        if (*tok == '\0')
            return -1;
        if (count >= max_out)
            return -1;

        uint32_t v = 0;
        if (parse_u32(tok, &v) != 0 || v > 0xFFu)
            return -1;
        out[count++] = (uint8_t)v;
    }

    if (count == 0u)
        return -1;

    *out_count = count;
    return 0;
}

static void print_hex_bytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        printf("%s0x%02X", (i == 0u) ? "" : " ", data[i]);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        print_usage(argv[0]);
        return 2;
    }

    const char *device_path = argv[1];
    const char *cmd = argv[2];

    crumbs_context_t ctx;
    crumbs_linux_i2c_t bus;
    int rc = crumbs_linux_init_controller(&ctx, &bus, device_path, 25000);
    if (rc != 0)
    {
        fprintf(stderr, "ERROR: init controller failed (%d)\n", rc);
        return 1;
    }

    if (strcmp(cmd, "scan") == 0)
    {
        uint8_t candidates[MAX_CANDIDATES];
        size_t candidate_count = 0u;
        if (parse_u8_csv(argv[3], candidates, MAX_CANDIDATES, &candidate_count) != 0)
        {
            fprintf(stderr, "ERROR: invalid candidate list '%s'\n", argv[3]);
            crumbs_linux_close(&bus);
            return 2;
        }
        for (size_t i = 0; i < candidate_count; ++i)
        {
            if (candidates[i] > 0x7Fu)
            {
                fprintf(stderr, "ERROR: candidate address out of 7-bit range: 0x%02X\n",
                        candidates[i]);
                crumbs_linux_close(&bus);
                return 2;
            }
        }

        int strict = 1;
        if (argc >= 5)
        {
            if (strcmp(argv[4], "strict") == 0)
                strict = 1;
            else if (strcmp(argv[4], "non-strict") == 0)
                strict = 0;
            else
            {
                fprintf(stderr, "ERROR: scan mode must be strict or non-strict\n");
                crumbs_linux_close(&bus);
                return 2;
            }
        }

        uint8_t found[MAX_CANDIDATES];
        uint8_t types[MAX_CANDIDATES];
        int n = crumbs_controller_scan_for_crumbs_candidates(
            &ctx,
            candidates,
            candidate_count,
            strict,
            crumbs_linux_i2c_write,
            crumbs_linux_read,
            &bus,
            found,
            types,
            MAX_CANDIDATES,
            25000);

        if (n < 0)
        {
            fprintf(stderr, "ERROR: candidate scan failed (%d)\n", n);
            crumbs_linux_close(&bus);
            return 1;
        }

        printf("Found %d CRUMBS device(s):\n", n);
        for (int i = 0; i < n; ++i)
        {
            printf("  addr=0x%02X type=0x%02X\n", found[i], types[i]);
        }

        crumbs_linux_close(&bus);
        return 0;
    }

    uint8_t sensor_addr = 0u;
    if (parse_i2c_addr(argv[3], &sensor_addr) != 0)
    {
        fprintf(stderr, "ERROR: invalid sensor address '%s'\n", argv[3]);
        crumbs_linux_close(&bus);
        return 2;
    }

    crumbs_device_t sensor = {
        .ctx = &ctx,
        .addr = sensor_addr,
        .write_fn = crumbs_linux_i2c_write,
        .read_fn = crumbs_linux_read,
        .delay_fn = NULL,
        .io = &bus};

    if (strcmp(cmd, "read-u8") == 0 || strcmp(cmd, "read-u16") == 0 || strcmp(cmd, "read-ex") == 0)
    {
        if (argc < 6)
        {
            print_usage(argv[0]);
            crumbs_linux_close(&bus);
            return 2;
        }

        uint32_t len_v = 0u;
        if (parse_u32(argv[5], &len_v) != 0 || len_v == 0u || len_v > MAX_SENSOR_IO)
        {
            fprintf(stderr, "ERROR: invalid read length '%s'\n", argv[5]);
            crumbs_linux_close(&bus);
            return 2;
        }
        size_t read_len = (size_t)len_v;

        int require_rs = 1;
        if (argc >= 7 && parse_repeat_mode(argv[6], &require_rs) != 0)
        {
            fprintf(stderr, "ERROR: repeat mode must be repeat|norepeat\n");
            crumbs_linux_close(&bus);
            return 2;
        }

        uint8_t out[MAX_SENSOR_IO];
        memset(out, 0, sizeof(out));

        if (strcmp(cmd, "read-u8") == 0)
        {
            uint8_t reg = 0u;
            if (parse_u8_byte(argv[4], &reg) != 0)
            {
                fprintf(stderr, "ERROR: invalid u8 register '%s'\n", argv[4]);
                crumbs_linux_close(&bus);
                return 2;
            }
            rc = crumbs_i2c_dev_read_reg_u8(
                &sensor, reg, out, read_len, 25000, require_rs, crumbs_linux_write_then_read);
        }
        else
        {
            if (strcmp(cmd, "read-u16") == 0)
            {
                uint16_t reg = 0u;
                if (parse_u16(argv[4], &reg) != 0)
                {
                    fprintf(stderr, "ERROR: invalid u16 register '%s'\n", argv[4]);
                    crumbs_linux_close(&bus);
                    return 2;
                }
                rc = crumbs_i2c_dev_read_reg_u16be(
                    &sensor, reg, out, read_len, 25000, require_rs, crumbs_linux_write_then_read);
            }
            else
            {
                uint8_t reg[MAX_SENSOR_IO];
                size_t reg_len = 0u;
                if (parse_u8_csv(argv[4], reg, MAX_SENSOR_IO, &reg_len) != 0)
                {
                    fprintf(stderr, "ERROR: invalid reg csv '%s'\n", argv[4]);
                    crumbs_linux_close(&bus);
                    return 2;
                }
                rc = crumbs_i2c_dev_read_reg_ex(
                    &sensor, reg, reg_len, out, read_len, 25000, require_rs, crumbs_linux_write_then_read);
            }
        }

        if (rc != CRUMBS_I2C_DEV_OK)
        {
            fprintf(stderr, "ERROR: sensor read failed (%d)\n", rc);
            crumbs_linux_close(&bus);
            return 1;
        }

        printf("Read %zu byte(s): ", read_len);
        print_hex_bytes(out, read_len);
        crumbs_linux_close(&bus);
        return 0;
    }

    if (strcmp(cmd, "write-u8") == 0 || strcmp(cmd, "write-u16") == 0 || strcmp(cmd, "write-ex") == 0)
    {
        if (argc < 6)
        {
            print_usage(argv[0]);
            crumbs_linux_close(&bus);
            return 2;
        }

        uint8_t payload[MAX_SENSOR_IO];
        size_t payload_len = 0u;
        if (parse_u8_csv(argv[5], payload, MAX_SENSOR_IO, &payload_len) != 0)
        {
            fprintf(stderr, "ERROR: invalid data csv '%s'\n", argv[5]);
            crumbs_linux_close(&bus);
            return 2;
        }

        if (strcmp(cmd, "write-u8") == 0)
        {
            uint8_t reg = 0u;
            if (parse_u8_byte(argv[4], &reg) != 0)
            {
                fprintf(stderr, "ERROR: invalid u8 register '%s'\n", argv[4]);
                crumbs_linux_close(&bus);
                return 2;
            }
            rc = crumbs_i2c_dev_write_reg_u8(&sensor, reg, payload, payload_len);
        }
        else if (strcmp(cmd, "write-u16") == 0)
        {
            uint16_t reg = 0u;
            if (parse_u16(argv[4], &reg) != 0)
            {
                fprintf(stderr, "ERROR: invalid u16 register '%s'\n", argv[4]);
                crumbs_linux_close(&bus);
                return 2;
            }
            rc = crumbs_i2c_dev_write_reg_u16be(&sensor, reg, payload, payload_len);
        }
        else
        {
            uint8_t reg[MAX_SENSOR_IO];
            size_t reg_len = 0u;
            if (parse_u8_csv(argv[4], reg, MAX_SENSOR_IO, &reg_len) != 0)
            {
                fprintf(stderr, "ERROR: invalid reg csv '%s'\n", argv[4]);
                crumbs_linux_close(&bus);
                return 2;
            }
            rc = crumbs_i2c_dev_write_reg_ex(&sensor, reg, reg_len, payload, payload_len);
        }

        if (rc != CRUMBS_I2C_DEV_OK)
        {
            fprintf(stderr, "ERROR: sensor write failed (%d)\n", rc);
            crumbs_linux_close(&bus);
            return 1;
        }

        printf("Write OK (%zu byte(s))\n", payload_len);
        crumbs_linux_close(&bus);
        return 0;
    }

    print_usage(argv[0]);
    crumbs_linux_close(&bus);
    return 2;
}
