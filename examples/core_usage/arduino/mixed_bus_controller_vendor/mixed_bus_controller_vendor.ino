#include <crumbs.h>
#include <crumbs_arduino.h>

#include <Wire.h>

#include <ezo_do.h>
#include <ezo_ph.h>
#include <ezo_i2c_arduino_wire.h>

#include <SparkFunBME280.h>

#include <math.h>

#include "config.h"

static crumbs_context_t g_ctx;
static uint32_t g_last_heartbeat_ms = 0;
static bool g_led_state = false;
static uint32_t g_last_status_ms = 0;

static ezo_arduino_wire_context_t g_ezo_wire_ctx;
static ezo_i2c_device_t g_ph_device;
static ezo_i2c_device_t g_do_device;
static ezo_do_output_config_t g_do_output_config;
static bool g_ezo_ready = false;
static bool g_do_output_config_ready = false;
static uint32_t g_ezo_started_ms = 0;

static BME280 g_bosch_sensors[MAX_BOSCH_SENSORS];
static bool g_bosch_present[MAX_BOSCH_SENSORS] = {false};

static_assert(BOSCH_SENSOR_COUNT <= MAX_BOSCH_SENSORS,
              "BOSCH_SENSOR_COUNT must be <= MAX_BOSCH_SENSORS");

static void heartbeat_tick()
{
    const uint32_t now = millis();
    if ((uint32_t)(now - g_last_heartbeat_ms) < HEARTBEAT_INTERVAL_MS)
        return;

    g_last_heartbeat_ms = now;
    g_led_state = !g_led_state;
    digitalWrite(LED_BUILTIN, g_led_state ? HIGH : LOW);
}

static void print_hex_u8(uint8_t value)
{
    if (value < 0x10)
        Serial.print('0');
    Serial.print(value, HEX);
}

static void print_hex_bytes(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i)
    {
        if (i > 0u)
            Serial.print(' ');
        Serial.print(F("0x"));
        print_hex_u8(data[i]);
    }
}

static size_t bosch_sensor_count_effective()
{
    const size_t available = sizeof(kBoschSensorAddrs) / sizeof(kBoschSensorAddrs[0]);
    const size_t configured = BOSCH_SENSOR_COUNT;
    if (configured < available)
        return configured;
    return available;
}

static size_t crumbs_candidate_count()
{
    return sizeof(kCrumbsCandidates) / sizeof(kCrumbsCandidates[0]);
}

static bool ezo_settle_elapsed()
{
    return (uint32_t)(millis() - g_ezo_started_ms) >= EZO_STARTUP_SETTLE_MS;
}

static int scan_crumbs(uint8_t *found, uint8_t *types, size_t max_found)
{
    return crumbs_controller_scan_for_crumbs_candidates(
        &g_ctx,
        kCrumbsCandidates,
        crumbs_candidate_count(),
        1,
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
    Serial.print(F("CRUMBS scan result: "));
    Serial.println(count);

    if (count <= 0)
        return;

    for (int i = 0; i < count && (size_t)i < max_found; ++i)
    {
        Serial.print(F("  addr=0x"));
        print_hex_u8(found[i]);
        Serial.print(F(" type=0x"));
        print_hex_u8(types[i]);
        Serial.println();
    }
}

static void print_crumbs_reply(uint8_t addr, uint8_t type_id)
{
    crumbs_message_t query = {0};
    query.type_id = type_id;
    query.opcode = CRUMBS_CMD_SET_REPLY;
    query.data_len = 1;
    query.data[0] = CRUMBS_QUERY_OPCODE;

    Serial.print(F("CRUMBS addr=0x"));
    print_hex_u8(addr);
    Serial.print(F(" type=0x"));
    print_hex_u8(type_id);

    int send_rc = crumbs_controller_send(&g_ctx, addr, &query, crumbs_arduino_wire_write, NULL);
    if (send_rc != 0)
    {
        Serial.print(F(" send_rc="));
        Serial.println(send_rc);
        return;
    }

    delay(CRUMBS_QUERY_DELAY_MS);

    uint8_t frame[CRUMBS_READ_BUFFER_LEN];
    int read_n = crumbs_arduino_read(NULL, addr, frame, sizeof(frame), CRUMBS_READ_TIMEOUT_US);
    if (read_n <= 0)
    {
        Serial.print(F(" read_n="));
        Serial.println(read_n);
        return;
    }

    crumbs_message_t reply;
    int decode_rc = crumbs_decode_message(frame, (size_t)read_n, &reply, &g_ctx);
    if (decode_rc != 0)
    {
        Serial.print(F(" decode_rc="));
        Serial.print(decode_rc);
        Serial.print(F(" raw="));
        print_hex_bytes(frame, (size_t)read_n);
        Serial.println();
        return;
    }

    Serial.print(F(" reply_op=0x"));
    print_hex_u8(reply.opcode);
    Serial.print(F(" len="));
    Serial.print(reply.data_len);
    Serial.print(F(" data="));
    print_hex_bytes(reply.data, reply.data_len);

    if (reply.data_len >= 5u && reply.opcode == CRUMBS_QUERY_OPCODE)
    {
        uint16_t crumbs_ver = (uint16_t)reply.data[0] | ((uint16_t)reply.data[1] << 8);
        Serial.print(F(" crumbs_ver=0x"));
        if (crumbs_ver < 0x1000u)
            Serial.print('0');
        if (crumbs_ver < 0x0100u)
            Serial.print('0');
        if (crumbs_ver < 0x0010u)
            Serial.print('0');
        Serial.print(crumbs_ver, HEX);
        Serial.print(F(" module="));
        Serial.print(reply.data[2]);
        Serial.print(F("."));
        Serial.print(reply.data[3]);
        Serial.print(F("."));
        Serial.print(reply.data[4]);
    }
    Serial.println();
}

static bool init_ezo_devices()
{
    ezo_result_t rc = ezo_arduino_wire_context_init(&g_ezo_wire_ctx, &Wire);
    if (rc != EZO_OK)
    {
        Serial.print(F("EZO init wire_ctx_rc="));
        Serial.print((int)rc);
        Serial.print(F(" ("));
        Serial.print(ezo_result_name(rc));
        Serial.println(F(")"));
        return false;
    }

    rc = ezo_device_init(&g_ph_device, EZO_PH_ADDR, ezo_arduino_wire_transport(), &g_ezo_wire_ctx);
    if (rc != EZO_OK)
    {
        Serial.print(F("EZO init ph_dev_rc="));
        Serial.print((int)rc);
        Serial.print(F(" ("));
        Serial.print(ezo_result_name(rc));
        Serial.println(F(")"));
        return false;
    }

    rc = ezo_device_init(&g_do_device, EZO_DO_ADDR, ezo_arduino_wire_transport(), &g_ezo_wire_ctx);
    if (rc != EZO_OK)
    {
        Serial.print(F("EZO init do_dev_rc="));
        Serial.print((int)rc);
        Serial.print(F(" ("));
        Serial.print(ezo_result_name(rc));
        Serial.println(F(")"));
        return false;
    }

    g_ezo_started_ms = millis();
    return true;
}

static bool refresh_do_output_config(ezo_result_t *send_rc_out,
                                     ezo_result_t *read_rc_out,
                                     ezo_device_status_t *status_out)
{
    ezo_timing_hint_t hint = {0};

    ezo_result_t send_rc = ezo_do_send_output_query_i2c(&g_do_device, &hint);
    if (send_rc_out)
        *send_rc_out = send_rc;
    if (send_rc != EZO_OK)
        return false;

    delay(hint.wait_ms);

    ezo_result_t read_rc = ezo_do_read_output_config_i2c(&g_do_device, &g_do_output_config);
    ezo_device_status_t status = ezo_device_get_last_status(&g_do_device);
    if (read_rc_out)
        *read_rc_out = read_rc;
    if (status_out)
        *status_out = status;
    if (read_rc != EZO_OK)
        return false;

    g_do_output_config_ready = true;
    return true;
}

static void print_ezo_ph_status()
{
    Serial.print(F("EZO pH addr=0x"));
    print_hex_u8(EZO_PH_ADDR);

    if (!g_ezo_ready)
    {
        Serial.println(F(" init=not_ready"));
        return;
    }

    if (!ezo_settle_elapsed())
    {
        Serial.println(F(" startup_settle=pending"));
        return;
    }

    ezo_timing_hint_t hint = {0};
    ezo_result_t send_rc = ezo_ph_send_read_i2c(&g_ph_device, &hint);
    if (send_rc != EZO_OK)
    {
        Serial.print(F(" send_rc="));
        Serial.print((int)send_rc);
        Serial.print(F(" send_name="));
        Serial.println(ezo_result_name(send_rc));
        return;
    }

    delay(hint.wait_ms);

    ezo_ph_reading_t reading;
    ezo_result_t read_rc = ezo_ph_read_response_i2c(&g_ph_device, &reading);
    ezo_device_status_t status = ezo_device_get_last_status(&g_ph_device);

    Serial.print(F(" read_rc="));
    Serial.print((int)read_rc);
    Serial.print(F(" read_name="));
    Serial.print(ezo_result_name(read_rc));
    Serial.print(F(" status="));
    Serial.print(ezo_device_status_name(status));

    if (read_rc == EZO_OK)
    {
        Serial.print(F(" ph="));
        Serial.print(reading.ph, 3);
    }

    Serial.println();
}

static void print_ezo_do_status()
{
    Serial.print(F("EZO DO addr=0x"));
    print_hex_u8(EZO_DO_ADDR);

    if (!g_ezo_ready)
    {
        Serial.println(F(" init=not_ready"));
        return;
    }

    if (!ezo_settle_elapsed())
    {
        Serial.println(F(" startup_settle=pending"));
        return;
    }

    if (!g_do_output_config_ready)
    {
        ezo_result_t cfg_send_rc = EZO_OK;
        ezo_result_t cfg_read_rc = EZO_OK;
        ezo_device_status_t cfg_status = EZO_STATUS_UNKNOWN;
        if (!refresh_do_output_config(&cfg_send_rc, &cfg_read_rc, &cfg_status))
        {
            Serial.print(F(" output_cfg=unavailable"));
            if (cfg_send_rc != EZO_OK)
            {
                Serial.print(F(" cfg_send_rc="));
                Serial.print((int)cfg_send_rc);
                Serial.print(F(" cfg_send_name="));
                Serial.print(ezo_result_name(cfg_send_rc));
            }
            else
            {
                Serial.print(F(" cfg_read_rc="));
                Serial.print((int)cfg_read_rc);
                Serial.print(F(" cfg_read_name="));
                Serial.print(ezo_result_name(cfg_read_rc));
                Serial.print(F(" cfg_status="));
                Serial.print(ezo_device_status_name(cfg_status));
            }
            Serial.println();
            return;
        }
    }

    ezo_timing_hint_t hint = {0};
    ezo_result_t send_rc = ezo_do_send_read_i2c(&g_do_device, &hint);
    if (send_rc != EZO_OK)
    {
        Serial.print(F(" send_rc="));
        Serial.print((int)send_rc);
        Serial.print(F(" send_name="));
        Serial.println(ezo_result_name(send_rc));
        return;
    }

    delay(hint.wait_ms);

    ezo_do_reading_t reading;
    ezo_result_t read_rc = ezo_do_read_response_i2c(&g_do_device, g_do_output_config.enabled_mask, &reading);
    ezo_device_status_t status = ezo_device_get_last_status(&g_do_device);

    Serial.print(F(" output_mask=0x"));
    if (g_do_output_config.enabled_mask < 0x10u)
        Serial.print('0');
    Serial.print((unsigned long)g_do_output_config.enabled_mask, HEX);

    Serial.print(F(" read_rc="));
    Serial.print((int)read_rc);
    Serial.print(F(" read_name="));
    Serial.print(ezo_result_name(read_rc));
    Serial.print(F(" status="));
    Serial.print(ezo_device_status_name(status));

    if (read_rc == EZO_OK)
    {
        Serial.print(F(" present_mask=0x"));
        if (reading.present_mask < 0x10u)
            Serial.print('0');
        Serial.print((unsigned long)reading.present_mask, HEX);

        if ((reading.present_mask & EZO_DO_OUTPUT_MG_L) != 0u)
        {
            Serial.print(F(" mg_l="));
            Serial.print(reading.milligrams_per_liter, 3);
        }
        if ((reading.present_mask & EZO_DO_OUTPUT_PERCENT_SATURATION) != 0u)
        {
            Serial.print(F(" sat_pct="));
            Serial.print(reading.percent_saturation, 3);
        }
    }

    Serial.println();
}

static bool begin_bosch_sensor(size_t index)
{
    if (index >= MAX_BOSCH_SENSORS)
        return false;

    g_bosch_sensors[index].setI2CAddress(kBoschSensorAddrs[index]);
    g_bosch_present[index] = (g_bosch_sensors[index].beginI2C(Wire) != 0);
    return g_bosch_present[index];
}

static void print_bosch_sensor_status(size_t index)
{
    const size_t sensor_count = bosch_sensor_count_effective();
    if (index >= sensor_count)
        return;

    const uint8_t addr = kBoschSensorAddrs[index];
    Serial.print(F("Bosch addr=0x"));
    print_hex_u8(addr);

    if (!g_bosch_present[index])
    {
        (void)begin_bosch_sensor(index);
    }

    if (!g_bosch_present[index])
    {
        Serial.println(F(" init=fail"));
        return;
    }

    const float temp_c = g_bosch_sensors[index].readTempC();
    const float pressure_pa = g_bosch_sensors[index].readFloatPressure();
    const float humidity = g_bosch_sensors[index].readFloatHumidity();

    Serial.print(F(" temp_c="));
    if (isnan(temp_c))
        Serial.print(F("nan"));
    else
        Serial.print(temp_c, 2);

    Serial.print(F(" pressure_pa="));
    if (isnan(pressure_pa))
        Serial.print(F("nan"));
    else
        Serial.print(pressure_pa, 2);

    Serial.print(F(" pressure_hpa="));
    if (isnan(pressure_pa))
        Serial.print(F("nan"));
    else
        Serial.print(pressure_pa / 100.0f, 2);

    Serial.print(F(" humidity_pct="));
    if (isnan(humidity))
        Serial.print(F("NA"));
    else
        Serial.print(humidity, 2);

    Serial.print(F(" model_hint="));
    if (isnan(humidity))
        Serial.print(F("BMP280_or_no_humidity"));
    else
        Serial.print(F("BME280"));

    Serial.println();
}

static void run_status_pass()
{
    uint8_t found[MAX_CRUMBS_FOUND] = {0};
    uint8_t types[MAX_CRUMBS_FOUND] = {0};

    int scan_count = scan_crumbs(found, types, MAX_CRUMBS_FOUND);
    print_scan_result(found, types, scan_count, MAX_CRUMBS_FOUND);

    if (scan_count > 0)
    {
        for (int i = 0; i < scan_count && (size_t)i < MAX_CRUMBS_FOUND; ++i)
        {
            print_crumbs_reply(found[i], types[i]);
        }
    }

    print_ezo_ph_status();
    print_ezo_do_status();

    const size_t sensor_count = bosch_sensor_count_effective();
    for (size_t i = 0; i < sensor_count; ++i)
    {
        print_bosch_sensor_status(i);
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

    g_ezo_ready = init_ezo_devices();

    const size_t sensor_count = bosch_sensor_count_effective();
    for (size_t i = 0; i < sensor_count; ++i)
    {
        g_bosch_present[i] = begin_bosch_sensor(i);
    }

    Serial.println(F("CRUMBS mixed-bus vendor controller example"));
    Serial.println(F("=== Validation pass (once at startup) ==="));
    run_status_pass();
}

void loop()
{
    heartbeat_tick();

    if ((uint32_t)(millis() - g_last_status_ms) < STATUS_INTERVAL_MS)
        return;

    g_last_status_ms = millis();
    Serial.println(F("=== Status pass ==="));
    run_status_pass();
}
