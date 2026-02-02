/**
 * @file main.cpp
 * @brief Calculator peripheral using CRUMBS handler pattern.
 *
 * This peripheral demonstrates a function-style interface with:
 * - SET operations (0x01-0x04): Execute arithmetic commands (ADD, SUB, MUL, DIV)
 * - GET operations (0x80-0x8D): Retrieve results and history via SET_REPLY
 * - Circular history buffer storing last 12 operations
 * - Pure software implementation (no hardware required)
 *
 * Hardware:
 * - Arduino Nano (ATmega328P)
 * - I2C address: 0x10
 * - I2C pins: A4 (SDA), A5 (SCL)
 *
 * Operations:
 * - CALC_OP_ADD (0x01): Add two u32 values
 * - CALC_OP_SUB (0x02): Subtract two u32 values
 * - CALC_OP_MUL (0x03): Multiply two u32 values
 * - CALC_OP_DIV (0x04): Divide two u32 values (division by zero returns 0xFFFFFFFF)
 * - CALC_OP_GET_RESULT (0x80): Returns last result
 * - CALC_OP_GET_HIST_META (0x81): Returns history metadata (count, write_pos)
 * - CALC_OP_GET_HIST_0..11 (0x82-0x8D): Returns specific history entry
 */

#include <Arduino.h>
#include <Wire.h>
#include <crumbs.h>
#include <crumbs_arduino.h>
#include <crumbs_message_helpers.h>

/* Include contract header (from parent directory) */
#include "calculator_ops.h"

/* ============================================================================
 * Configuration
 * ============================================================================ */

#define PERIPHERAL_ADDR 0x10
#define HISTORY_SIZE 12

/* ============================================================================
 * History Entry Structure
 * ============================================================================ */

/**
 * @brief History entry: 16 bytes
 * - op: Operation name (4 bytes, null-terminated)
 * - a: First operand (4 bytes, little-endian)
 * - b: Second operand (4 bytes, little-endian)
 * - result: Result (4 bytes, little-endian)
 */
typedef struct
{
    char op[4];      /* "ADD\0", "SUB\0", "MUL\0", "DIV\0" */
    uint32_t a;      /* First operand */
    uint32_t b;      /* Second operand */
    uint32_t result; /* Result of operation */
} calc_history_entry_t;

/* ============================================================================
 * State
 * ============================================================================ */

static crumbs_context_t ctx;
static uint32_t g_last_result = 0;                   /* Last calculation result */
static calc_history_entry_t g_history[HISTORY_SIZE]; /* Circular history buffer */
static uint8_t g_history_count = 0;                  /* Valid entries (0-12) */
static uint8_t g_history_write_pos = 0;              /* Next write slot (0-11) */

/* ============================================================================
 * Helper: Append to History
 * ============================================================================ */

/**
 * @brief Append operation to circular history buffer.
 *
 * @param op     Operation name (4 bytes, e.g., "ADD\0")
 * @param a      First operand
 * @param b      Second operand
 * @param result Result of operation
 */
static void append_to_history(const char *op, uint32_t a, uint32_t b, uint32_t result)
{
    calc_history_entry_t *entry = &g_history[g_history_write_pos];

    /* Copy operation name (ensure null-termination) */
    strncpy(entry->op, op, 4);
    entry->op[3] = '\0';

    /* Store operands and result */
    entry->a = a;
    entry->b = b;
    entry->result = result;

    /* Update circular buffer pointers */
    g_history_write_pos = (g_history_write_pos + 1) % HISTORY_SIZE;
    if (g_history_count < HISTORY_SIZE)
    {
        g_history_count++;
    }
}

/* ============================================================================
 * Handler: ADD (CALC_OP_ADD)
 * ============================================================================ */

/**
 * @brief Handler for ADD operation - adds two u32 values.
 */
static void handler_add(crumbs_context_t *ctx, uint8_t opcode,
                        const uint8_t *data, uint8_t data_len, void *user_data)
{
    (void)ctx;
    (void)opcode;
    (void)user_data;

    uint32_t a, b;

    /* Extract operands */
    if (crumbs_msg_read_u32(data, data_len, 0, &a) != 0 ||
        crumbs_msg_read_u32(data, data_len, 4, &b) != 0)
    {
        Serial.println(F("ADD: Invalid payload"));
        return;
    }

    /* Perform addition */
    uint32_t result = a + b;
    g_last_result = result;

    /* Append to history */
    append_to_history("ADD", a, b, result);

    /* Log to serial */
    Serial.print(F("ADD: "));
    Serial.print(a);
    Serial.print(F(" + "));
    Serial.print(b);
    Serial.print(F(" = "));
    Serial.println(result);
}

/* ============================================================================
 * Handler: SUB (CALC_OP_SUB)
 * ============================================================================ */

/**
 * @brief Handler for SUB operation - subtracts b from a.
 */
static void handler_sub(crumbs_context_t *ctx, uint8_t opcode,
                        const uint8_t *data, uint8_t data_len, void *user_data)
{
    (void)ctx;
    (void)opcode;
    (void)user_data;

    uint32_t a, b;

    /* Extract operands */
    if (crumbs_msg_read_u32(data, data_len, 0, &a) != 0 ||
        crumbs_msg_read_u32(data, data_len, 4, &b) != 0)
    {
        Serial.println(F("SUB: Invalid payload"));
        return;
    }

    /* Perform subtraction */
    uint32_t result = a - b;
    g_last_result = result;

    /* Append to history */
    append_to_history("SUB", a, b, result);

    /* Log to serial */
    Serial.print(F("SUB: "));
    Serial.print(a);
    Serial.print(F(" - "));
    Serial.print(b);
    Serial.print(F(" = "));
    Serial.println(result);
}

/* ============================================================================
 * Handler: MUL (CALC_OP_MUL)
 * ============================================================================ */

/**
 * @brief Handler for MUL operation - multiplies two u32 values.
 */
static void handler_mul(crumbs_context_t *ctx, uint8_t opcode,
                        const uint8_t *data, uint8_t data_len, void *user_data)
{
    (void)ctx;
    (void)opcode;
    (void)user_data;

    uint32_t a, b;

    /* Extract operands */
    if (crumbs_msg_read_u32(data, data_len, 0, &a) != 0 ||
        crumbs_msg_read_u32(data, data_len, 4, &b) != 0)
    {
        Serial.println(F("MUL: Invalid payload"));
        return;
    }

    /* Perform multiplication */
    uint32_t result = a * b;
    g_last_result = result;

    /* Append to history */
    append_to_history("MUL", a, b, result);

    /* Log to serial */
    Serial.print(F("MUL: "));
    Serial.print(a);
    Serial.print(F(" * "));
    Serial.print(b);
    Serial.print(F(" = "));
    Serial.println(result);
}

/* ============================================================================
 * Handler: DIV (CALC_OP_DIV)
 * ============================================================================ */

/**
 * @brief Handler for DIV operation - divides a by b (integer division).
 * Division by zero returns 0xFFFFFFFF.
 */
static void handler_div(crumbs_context_t *ctx, uint8_t opcode,
                        const uint8_t *data, uint8_t data_len, void *user_data)
{
    (void)ctx;
    (void)opcode;
    (void)user_data;

    uint32_t a, b;

    /* Extract operands */
    if (crumbs_msg_read_u32(data, data_len, 0, &a) != 0 ||
        crumbs_msg_read_u32(data, data_len, 4, &b) != 0)
    {
        Serial.println(F("DIV: Invalid payload"));
        return;
    }

    /* Perform division (handle division by zero) */
    uint32_t result;
    if (b == 0)
    {
        result = 0xFFFFFFFF;
        Serial.print(F("DIV: "));
        Serial.print(a);
        Serial.print(F(" / "));
        Serial.print(b);
        Serial.println(F(" = ERROR (div by zero)"));
    }
    else
    {
        result = a / b;
        Serial.print(F("DIV: "));
        Serial.print(a);
        Serial.print(F(" / "));
        Serial.print(b);
        Serial.print(F(" = "));
        Serial.println(result);
    }

    g_last_result = result;

    /* Append to history */
    append_to_history("DIV", a, b, result);
}

/* ============================================================================
 * Request Handler (GET operations via SET_REPLY)
 * ============================================================================ */

/**
 * @brief Request handler for I2C read operations.
 *
 * Switches on ctx->requested_opcode (set by controller via SET_REPLY).
 * Handles:
 * - CALC_OP_GET_RESULT: Returns last result (u32)
 * - CALC_OP_GET_HIST_META: Returns count and write_pos (2 bytes)
 * - CALC_OP_GET_HIST_0..11: Returns specific history entry (16 bytes)
 * - Default: Returns device info
 */
static void on_request(crumbs_context_t *ctx, crumbs_message_t *reply)
{
    switch (ctx->requested_opcode)
    {
    case 0: /* Version info per versioning.md convention */
        crumbs_build_version_reply(reply, CALC_TYPE_ID,
                                   CALC_MODULE_VER_MAJOR,
                                   CALC_MODULE_VER_MINOR,
                                   CALC_MODULE_VER_PATCH);
        break;

    case CALC_OP_GET_RESULT:
    {
        crumbs_msg_init(reply, CALC_TYPE_ID, CALC_OP_GET_RESULT);
        crumbs_msg_add_u32(reply, g_last_result);
        break;
    }

    case CALC_OP_GET_HIST_META:
    {
        crumbs_msg_init(reply, CALC_TYPE_ID, CALC_OP_GET_HIST_META);
        crumbs_msg_add_u8(reply, g_history_count);
        crumbs_msg_add_u8(reply, g_history_write_pos);
        break;
    }

    case CALC_OP_GET_HIST_0:
    case CALC_OP_GET_HIST_1:
    case CALC_OP_GET_HIST_2:
    case CALC_OP_GET_HIST_3:
    case CALC_OP_GET_HIST_4:
    case CALC_OP_GET_HIST_5:
    case CALC_OP_GET_HIST_6:
    case CALC_OP_GET_HIST_7:
    case CALC_OP_GET_HIST_8:
    case CALC_OP_GET_HIST_9:
    case CALC_OP_GET_HIST_10:
    case CALC_OP_GET_HIST_11:
    {
        uint8_t entry_idx = ctx->requested_opcode - CALC_OP_GET_HIST_0;

        crumbs_msg_init(reply, CALC_TYPE_ID, ctx->requested_opcode);

        /* Check if entry exists */
        if (entry_idx < g_history_count)
        {
            /* Copy 16-byte entry directly to reply data */
            memcpy(reply->data, &g_history[entry_idx], 16);
            reply->data_len = 16;
        }
        else
        {
            /* Entry doesn't exist - return empty message */
            reply->data_len = 0;
        }
        break;
    }

    default:
        /* Unknown opcode - return empty message */
        crumbs_msg_init(reply, CALC_TYPE_ID, ctx->requested_opcode);
        break;
    }
}

/* ============================================================================
 * Setup & Loop
 * ============================================================================ */

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(10);
    }

    Serial.println(F("\n=== CRUMBS Calculator Peripheral ==="));
    Serial.print(F("I2C Address: 0x"));
    Serial.println(PERIPHERAL_ADDR, HEX);
    Serial.print(F("Type ID: 0x"));
    Serial.println(CALC_TYPE_ID, HEX);
    Serial.println(F("Operations: ADD, SUB, MUL, DIV"));
    Serial.println(F("History: 12 entries (circular buffer)"));
    Serial.println();

    /* Initialize CRUMBS context */
    crumbs_arduino_init_peripheral(&ctx, PERIPHERAL_ADDR);

    /* Register SET operation handlers */
    crumbs_register_handler(&ctx, CALC_OP_ADD, handler_add, nullptr);
    crumbs_register_handler(&ctx, CALC_OP_SUB, handler_sub, nullptr);
    crumbs_register_handler(&ctx, CALC_OP_MUL, handler_mul, nullptr);
    crumbs_register_handler(&ctx, CALC_OP_DIV, handler_div, nullptr);

    /* Register GET operation callback */
    crumbs_set_callbacks(&ctx, nullptr, on_request, nullptr);

    Serial.println(F("Calculator peripheral ready!"));
    Serial.println(F("Waiting for I2C commands...\n"));
}

void loop()
{
    /* I2C communication handled by Wire library interrupts */
    /* Just keep peripheral alive */
    delay(100);
}
