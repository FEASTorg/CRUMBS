#include <crumbs.h>
#include <crumbs_arduino.h>

// Configure for your hardware setup.
#define SENSOR_ADDR 0x76
#define SENSOR_WHOAMI_REG 0xD0

// Only probe known CRUMBS addresses on mixed buses.
static const uint8_t kCrumbsCandidates[] = {0x20, 0x21, 0x30};

static crumbs_context_t g_ctx;
static crumbs_device_t g_sensor = {
    &g_ctx,
    SENSOR_ADDR,
    crumbs_arduino_wire_write,
    crumbs_arduino_read,
    crumbs_arduino_delay_us,
    NULL};

static void print_hex_u8(uint8_t v)
{
    if (v < 0x10)
        Serial.print('0');
    Serial.print(v, HEX);
}

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
    }

    crumbs_arduino_init_controller(&g_ctx);
    Serial.println("CRUMBS mixed-bus controller example");
}

void loop()
{
    uint8_t found[8];
    uint8_t types[8];

    int n = crumbs_controller_scan_for_crumbs_candidates(
        &g_ctx,
        kCrumbsCandidates,
        sizeof(kCrumbsCandidates),
        1, /* strict */
        crumbs_arduino_wire_write,
        crumbs_arduino_read,
        NULL,
        found,
        types,
        8,
        10000);

    Serial.print("CRUMBS scan result: ");
    Serial.println(n);
    for (int i = 0; i < n && i < 8; ++i)
    {
        Serial.print("  addr=0x");
        print_hex_u8(found[i]);
        Serial.print(" type=0x");
        print_hex_u8(types[i]);
        Serial.println();
    }

    uint8_t whoami = 0;
    int rc = crumbs_i2c_dev_read_reg_u8(
        &g_sensor,
        SENSOR_WHOAMI_REG,
        &whoami,
        1,
        10000,
        1, /* require repeated-start */
        crumbs_arduino_write_then_read);

    Serial.print("Sensor read rc=");
    Serial.print(rc);
    if (rc == CRUMBS_I2C_DEV_OK)
    {
        Serial.print(" WHOAMI=0x");
        print_hex_u8(whoami);
    }
    Serial.println();

    delay(2000);
}

