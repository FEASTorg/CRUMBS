/**
 * @file handler_peripheral_servo.ino
 * @brief Example CRUMBS servo controller peripheral using command interface.
 *
 * This sketch demonstrates:
 * - Using crumbs_msg.h for payload parsing
 * - Using servo_commands.h for command definitions
 * - Registering handlers with crumbs_register_handler()
 * - Responding to I2C read requests
 *
 * Hardware:
 * - Arduino Nano (or compatible)
 * - 2x SG90 servos on pins D9, D10
 * - I2C address: 0x09
 *
 * Memory optimization:
 * - Define CRUMBS_MAX_HANDLERS=8 to reduce RAM usage
 * - In PlatformIO: build_flags = -DCRUMBS_MAX_HANDLERS=8
 *
 * Serial Test Mode:
 * - Set ENABLE_SERIAL_TEST to 1 to test hardware via serial commands
 * - Set to 0 for normal I2C peripheral operation
 */

/* ============================================================================
 * Build Configuration
 * ============================================================================ */

/**
 * @brief Enable serial test mode for hardware testing.
 *
 * When 1: Serial commands control servos directly (I2C disabled)
 * When 0: Normal I2C peripheral mode
 */
#define ENABLE_SERIAL_TEST 0

/**
 * @brief Enable CRUMBS debug output via Serial.
 *
 * When 1: Prints debug messages for I2C receive/transmit
 * When 0: No debug output (smaller code size)
 */
#define ENABLE_CRUMBS_DEBUG 0

/*
 * NOTE: To reduce handler table size, add to platformio.ini:
 *   build_flags = -DCRUMBS_MAX_HANDLERS=8
 *
 * Defining it here does NOT work because Arduino precompiles
 * the library separately from the sketch.
 * Default is 16 handlers which uses ~70 bytes on AVR.
 */

/* Enable CRUMBS debug if configured */
#if ENABLE_CRUMBS_DEBUG
#define CRUMBS_DEBUG
#endif

#include <Servo.h>
#include <crumbs.h>
#include <crumbs_arduino.h>
#include <crumbs_msg.h>

/* Include for command definitions */
#include "servo_commands.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define I2C_ADDRESS 0x09

static const uint8_t SERVO_PINS[] = {9, 10};
static const uint8_t NUM_SERVOS = 2;

/* ============================================================================
 * State
 * ============================================================================ */

static Servo servos[NUM_SERVOS];
static uint8_t servo_angles[NUM_SERVOS] = {90, 90}; /* Current angles */

#if !ENABLE_SERIAL_TEST
static crumbs_context_t ctx;
#endif

/* ============================================================================
 * Hardware Control
 * ============================================================================ */

/**
 * @brief Set servo to specified angle.
 */
static void set_servo_angle(uint8_t channel, uint8_t angle)
{
    if (channel < NUM_SERVOS && angle <= 180)
    {
        servo_angles[channel] = angle;
        servos[channel].write(angle);
    }
}

/**
 * @brief Sweep servo from current position to target.
 */
static void sweep_servo(uint8_t channel, uint8_t start, uint8_t end, uint8_t step_ms)
{
    if (channel >= NUM_SERVOS)
        return;

    int8_t direction = (end > start) ? 1 : -1;
    uint8_t current = start;

    set_servo_angle(channel, current);

    while (current != end)
    {
        delay(step_ms);
        current += direction;
        set_servo_angle(channel, current);
    }
}

#if !ENABLE_SERIAL_TEST
/* ============================================================================
 * Command Handlers (I2C mode only)
 * ============================================================================ */

/**
 * @brief Handler for SERVO_CMD_SET_ANGLE.
 */
static void handle_set_angle(crumbs_context_t *ctx, uint8_t cmd,
                             const uint8_t *data, uint8_t len, void *user)
{
    (void)ctx;
    (void)cmd;
    (void)user;

    uint8_t channel, angle;
    if (crumbs_msg_read_u8(data, len, 0, &channel) == 0 &&
        crumbs_msg_read_u8(data, len, 1, &angle) == 0)
    {

        set_servo_angle(channel, angle);

        Serial.print(F("SET_ANGLE: ch"));
        Serial.print(channel);
        Serial.print(F(" = "));
        Serial.print(angle);
        Serial.println(F("°"));
    }
}

/**
 * @brief Handler for SERVO_CMD_SET_BOTH.
 */
static void handle_set_both(crumbs_context_t *ctx, uint8_t cmd,
                            const uint8_t *data, uint8_t len, void *user)
{
    (void)ctx;
    (void)cmd;
    (void)user;

    uint8_t angle0, angle1;
    if (crumbs_msg_read_u8(data, len, 0, &angle0) == 0 &&
        crumbs_msg_read_u8(data, len, 1, &angle1) == 0)
    {

        set_servo_angle(0, angle0);
        set_servo_angle(1, angle1);

        Serial.print(F("SET_BOTH: "));
        Serial.print(angle0);
        Serial.print(F("°, "));
        Serial.print(angle1);
        Serial.println(F("°"));
    }
}

/**
 * @brief Handler for SERVO_CMD_SWEEP.
 */
static void handle_sweep(crumbs_context_t *ctx, uint8_t cmd,
                         const uint8_t *data, uint8_t len, void *user)
{
    (void)ctx;
    (void)cmd;
    (void)user;

    uint8_t channel, start, end, step_ms;
    if (crumbs_msg_read_u8(data, len, 0, &channel) == 0 &&
        crumbs_msg_read_u8(data, len, 1, &start) == 0 &&
        crumbs_msg_read_u8(data, len, 2, &end) == 0 &&
        crumbs_msg_read_u8(data, len, 3, &step_ms) == 0)
    {

        Serial.print(F("SWEEP: ch"));
        Serial.print(channel);
        Serial.print(F(" "));
        Serial.print(start);
        Serial.print(F("° -> "));
        Serial.print(end);
        Serial.print(F("° @ "));
        Serial.print(step_ms);
        Serial.println(F("ms/step"));

        sweep_servo(channel, start, end, step_ms);
    }
}

/**
 * @brief Handler for SERVO_CMD_CENTER_ALL.
 */
static void handle_center_all(struct crumbs_context_s *ctx, uint8_t cmd,
                              const uint8_t *data, uint8_t len, void *user)
{
    (void)ctx;
    (void)cmd;
    (void)data;
    (void)len;
    (void)user;

    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        set_servo_angle(i, 90);
    }

    Serial.println(F("CENTER_ALL: all servos to 90°"));
}

/**
 * @brief Request handler for I2C read (GET_ANGLES response).
 */
static void handle_request(struct crumbs_context_s *ctx, crumbs_message_t *reply)
{
    (void)ctx;

    crumbs_msg_init(reply, SERVO_TYPE_ID, SERVO_CMD_GET_ANGLES);
    crumbs_msg_add_u8(reply, servo_angles[0]);
    crumbs_msg_add_u8(reply, servo_angles[1]);

    Serial.print(F("GET_ANGLES: "));
    Serial.print(servo_angles[0]);
    Serial.print(F("°, "));
    Serial.print(servo_angles[1]);
    Serial.println(F("°"));
}

#endif /* !ENABLE_SERIAL_TEST */

/* ============================================================================
 * Arduino Setup & Loop
 * ============================================================================ */

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }
    Serial.println(F("Starting Init"));

    /* Initialize servos */
    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        servos[i].attach(SERVO_PINS[i]);
        servos[i].write(90); /* Start centered */
    }

#if ENABLE_SERIAL_TEST
    /* Serial test mode - print help */
    serial_io_setup();
#else
    /* Verify struct size matches library (catches CRUMBS_MAX_HANDLERS mismatch) */
    if (sizeof(ctx) != crumbs_context_size())
    {
        Serial.println(F("ERROR: Context size mismatch!"));
        Serial.print(F("Sketch: "));
        Serial.print(sizeof(ctx));
        Serial.print(F("  Library: "));
        Serial.println(crumbs_context_size());
        Serial.println(F("Add to platformio.ini:"));
        Serial.println(F("  build_flags = -DCRUMBS_MAX_HANDLERS=16"));
        while (1)
        {
            delay(1000);
        } /* Halt */
    }

    /* Initialize CRUMBS peripheral */
    crumbs_arduino_init_peripheral(&ctx, I2C_ADDRESS);

    /* Set up request callback for I2C reads */
    crumbs_set_callbacks(&ctx, nullptr, handle_request, nullptr);

    /* Register command handlers */
    crumbs_register_handler(&ctx, SERVO_CMD_SET_ANGLE, handle_set_angle, nullptr);
    crumbs_register_handler(&ctx, SERVO_CMD_SET_BOTH, handle_set_both, nullptr);
    crumbs_register_handler(&ctx, SERVO_CMD_SWEEP, handle_sweep, nullptr);
    crumbs_register_handler(&ctx, SERVO_CMD_CENTER_ALL, handle_center_all, nullptr);

    Serial.println(F("=== Servo Peripheral Ready ==="));
    Serial.print(F("I2C Address: 0x"));
    Serial.println(I2C_ADDRESS, HEX);
#endif

    Serial.print(F("Servos on pins: "));
    for (uint8_t i = 0; i < NUM_SERVOS; i++)
    {
        Serial.print(SERVO_PINS[i]);
        if (i < NUM_SERVOS - 1)
            Serial.print(F(", "));
    }
    Serial.println();
}

void loop()
{
#if ENABLE_SERIAL_TEST
    serial_io_loop();
#endif
    /* I2C processing happens via callbacks */
}
