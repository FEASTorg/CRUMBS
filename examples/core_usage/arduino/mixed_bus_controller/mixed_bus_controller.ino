#include <crumbs.h>
#include <crumbs_arduino.h>
#include "config.h"

static crumbs_context_t g_ctx;
static uint32_t last_heartbeat_ms = 0;
static bool led_state = false;
static uint32_t last_status_ms = 0;

static void heartbeat_tick()
{
    const uint32_t now = millis();
    if ((uint32_t)(now - last_heartbeat_ms) < HEARTBEAT_INTERVAL_MS)
        return;

    last_heartbeat_ms = now;
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state ? HIGH : LOW);
}

static void print_hex_u8(uint8_t v)
{
    if (v < 0x10)
        Serial.print('0');
    Serial.print(v, HEX);
}

static void print_hex_bytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (i > 0u)
            Serial.print(' ');
        Serial.print("0x");
        print_hex_u8(data[i]);
    }
}

static const char *sensor_model_name(uint8_t chip_id)
{
    if (chip_id == BMP280_CHIP_ID)
        return "BMP280";
    if (chip_id == BME280_CHIP_ID)
        return "BME280";
    return "UNKNOWN";
}

static size_t sensor_count_effective()
{
    const size_t configured = SENSOR_COUNT;
    const size_t available = sizeof(kSensorAddrs) / sizeof(kSensorAddrs[0]);
    return (configured < available) ? configured : available;
}

static crumbs_device_t make_sensor_dev(uint8_t addr)
{
    crumbs_device_t dev = {
        &g_ctx,
        addr,
        crumbs_arduino_wire_write,
        crumbs_arduino_read,
        crumbs_arduino_delay_us,
        NULL};
    return dev;
}

static int scan_crumbs(uint8_t *found, uint8_t *types, size_t max_found)
{
    return crumbs_controller_scan_for_crumbs_candidates(
        &g_ctx,
        kCrumbsCandidates,
        sizeof(kCrumbsCandidates),
        1, /* strict */
        crumbs_arduino_wire_write,
        crumbs_arduino_read,
        NULL,
        found,
        types,
        max_found,
        CRUMBS_READ_TIMEOUT_US);
}

static void print_scan_result(const uint8_t *found, const uint8_t *types, int count, size_t max_found)
{
    Serial.print("CRUMBS scan result: ");
    Serial.println(count);

    if (count <= 0)
        return;

    for (int i = 0; i < count && (size_t)i < max_found; ++i)
    {
        Serial.print("  addr=0x");
        print_hex_u8(found[i]);
        Serial.print(" type=0x");
        print_hex_u8(types[i]);
        Serial.println();
    }
}

static void print_crumbs_reply(uint8_t addr, uint8_t type_id)
{
    crumbs_message_t query;
    query.type_id = type_id;
    query.opcode = CRUMBS_CMD_SET_REPLY;
    query.data_len = 1;
    query.data[0] = CRUMBS_QUERY_OPCODE;

    Serial.print("CRUMBS addr=0x");
    print_hex_u8(addr);
    Serial.print(" type=0x");
    print_hex_u8(type_id);

    int send_rc = crumbs_controller_send(&g_ctx, addr, &query, crumbs_arduino_wire_write, NULL);
    if (send_rc != 0)
    {
        Serial.print(" send_rc=");
        Serial.println(send_rc);
        return;
    }

    delay(CRUMBS_QUERY_DELAY_MS);

    uint8_t frame[CRUMBS_READ_BUFFER_LEN];
    int read_n = crumbs_arduino_read(NULL, addr, frame, sizeof(frame), CRUMBS_READ_TIMEOUT_US);
    if (read_n <= 0)
    {
        Serial.print(" read_n=");
        Serial.println(read_n);
        return;
    }

    crumbs_message_t reply;
    int dec_rc = crumbs_decode_message(frame, (size_t)read_n, &reply, &g_ctx);
    if (dec_rc != 0)
    {
        Serial.print(" decode_rc=");
        Serial.print(dec_rc);
        Serial.print(" raw=");
        print_hex_bytes(frame, (size_t)read_n);
        Serial.println();
        return;
    }

    Serial.print(" reply_op=0x");
    print_hex_u8(reply.opcode);
    Serial.print(" len=");
    Serial.print(reply.data_len);
    Serial.print(" data=");
    print_hex_bytes(reply.data, reply.data_len);

    if (reply.data_len >= 5u && reply.opcode == CRUMBS_QUERY_OPCODE)
    {
        uint16_t crumbs_ver = (uint16_t)reply.data[0] | ((uint16_t)reply.data[1] << 8);
        Serial.print(" crumbs_ver=0x");
        if (crumbs_ver < 0x1000u)
            Serial.print('0');
        if (crumbs_ver < 0x0100u)
            Serial.print('0');
        if (crumbs_ver < 0x0010u)
            Serial.print('0');
        Serial.print(crumbs_ver, HEX);
        Serial.print(" module=");
        Serial.print(reply.data[2]);
        Serial.print(".");
        Serial.print(reply.data[3]);
        Serial.print(".");
        Serial.print(reply.data[4]);
    }
    Serial.println();
}

static void configure_sensor_for_periodic_read(const crumbs_device_t *sensor, uint8_t chip_id)
{
    if (chip_id == BME280_CHIP_ID)
    {
        const uint8_t hum_cfg = SENSOR_CTRL_HUM_X1;
        (void)crumbs_i2c_dev_write_reg_u8(sensor, SENSOR_CTRL_HUM_REG, &hum_cfg, 1u);
    }

    const uint8_t meas_cfg = SENSOR_CTRL_MEAS_NORMAL_X1;
    (void)crumbs_i2c_dev_write_reg_u8(sensor, SENSOR_CTRL_MEAS_REG, &meas_cfg, 1u);
}

static void print_sensor_status(uint8_t addr, int configure_on_first_seen)
{
    crumbs_device_t sensor = make_sensor_dev(addr);
    uint8_t chip_id = 0;
    int chip_rc = crumbs_i2c_dev_read_reg_u8(
        &sensor,
        SENSOR_CHIP_ID_REG,
        &chip_id,
        1,
        SENSOR_READ_TIMEOUT_US,
        1, /* require repeated-start */
        crumbs_arduino_write_then_read);

    Serial.print("Sensor addr=0x");
    print_hex_u8(sensor.addr);
    Serial.print(" chip_rc=");
    Serial.print(chip_rc);
    if (chip_rc != CRUMBS_I2C_DEV_OK)
    {
        Serial.println();
        return;
    }

    if (configure_on_first_seen)
    {
        configure_sensor_for_periodic_read(&sensor, chip_id);
    }

    const size_t raw_len = (chip_id == BME280_CHIP_ID) ? BME280_RAW_LEN : BMP280_RAW_LEN;
    uint8_t raw[8] = {0};
    int raw_rc = crumbs_i2c_dev_read_reg_u8(
        &sensor,
        SENSOR_RAW_DATA_REG,
        raw,
        raw_len,
        SENSOR_READ_TIMEOUT_US,
        1, /* require repeated-start */
        crumbs_arduino_write_then_read);

    Serial.print(" chip_id=0x");
    print_hex_u8(chip_id);
    Serial.print(" model=");
    Serial.print(sensor_model_name(chip_id));
    Serial.print(" raw_rc=");
    Serial.print(raw_rc);

    if (raw_rc == CRUMBS_I2C_DEV_OK)
    {
        Serial.print(" raw=");
        print_hex_bytes(raw, raw_len);
    }
    Serial.println();
}

static void run_status_pass(int configure_sensors)
{
    uint8_t found[8] = {0};
    uint8_t types[8] = {0};
    int n = scan_crumbs(found, types, 8);
    print_scan_result(found, types, n, 8);

    if (n > 0)
    {
        for (int i = 0; i < n && i < 8; ++i)
        {
            print_crumbs_reply(found[i], types[i]);
        }
    }

    const size_t sensor_count = sensor_count_effective();
    for (size_t i = 0; i < sensor_count; ++i)
    {
        print_sensor_status(kSensorAddrs[i], configure_sensors);
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    while (!Serial)
    {
    }

    crumbs_arduino_init_controller(&g_ctx);
    Serial.println("CRUMBS mixed-bus controller example");
    Serial.println("=== Validation pass (once at startup) ===");
    run_status_pass(1);
}

void loop()
{
    heartbeat_tick();

    if ((uint32_t)(millis() - last_status_ms) < STATUS_INTERVAL_MS)
    {
        return;
    }
    last_status_ms = millis();

    Serial.println("=== Status pass ===");
    run_status_pass(0);
}
