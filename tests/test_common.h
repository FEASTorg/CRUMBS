/*
 * Common test infrastructure for CRUMBS unit tests.
 *
 * Provides standard assertion macros and helper functions to reduce
 * duplication and ensure consistent test conventions.
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "crumbs.h"

/* ============================================================================
 * Assertion Macros
 * ============================================================================
 *
 * All macros print the test name and condition on failure, then return 1.
 * Use these inside test functions that return int (0=pass, 1=fail).
 */

/**
 * @brief Assert that a condition is true.
 *
 * @param test_name Name of the current test for error messages.
 * @param cond Condition to check.
 * @param msg Message to print on failure.
 */
#define TEST_ASSERT(test_name, cond, msg)                                      \
    do                                                                         \
    {                                                                          \
        if (!(cond))                                                           \
        {                                                                      \
            fprintf(stderr, "%s: %s\n", (test_name), (msg));                   \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * @brief Assert that two integer values are equal.
 */
#define TEST_ASSERT_EQ(test_name, actual, expected, msg)                       \
    do                                                                         \
    {                                                                          \
        if ((actual) != (expected))                                            \
        {                                                                      \
            fprintf(stderr, "%s: %s (got %d, expected %d)\n", (test_name),     \
                    (msg), (int)(actual), (int)(expected));                    \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * @brief Assert that two size_t values are equal.
 */
#define TEST_ASSERT_SIZE_EQ(test_name, actual, expected, msg)                  \
    do                                                                         \
    {                                                                          \
        if ((actual) != (expected))                                            \
        {                                                                      \
            fprintf(stderr, "%s: %s (got %zu, expected %zu)\n", (test_name),   \
                    (msg), (size_t)(actual), (size_t)(expected));              \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * @brief Assert that a pointer is NULL.
 */
#define TEST_ASSERT_NULL(test_name, ptr, msg)                                  \
    do                                                                         \
    {                                                                          \
        if ((ptr) != NULL)                                                     \
        {                                                                      \
            fprintf(stderr, "%s: %s (expected NULL, got %p)\n", (test_name),   \
                    (msg), (void *)(ptr));                                     \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/**
 * @brief Assert that a pointer is not NULL.
 */
#define TEST_ASSERT_NOT_NULL(test_name, ptr, msg)                              \
    do                                                                         \
    {                                                                          \
        if ((ptr) == NULL)                                                     \
        {                                                                      \
            fprintf(stderr, "%s: %s (unexpected NULL)\n", (test_name), (msg)); \
            return 1;                                                          \
        }                                                                      \
    } while (0)

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/**
 * @brief Initialize a message with the given header and zero payload.
 *
 * Convenience function to reduce boilerplate in tests.
 *
 * @param msg Pointer to message to initialize.
 * @param type_id Type ID byte.
 * @param cmd Command type byte.
 */
static inline void test_msg_init(crumbs_message_t *msg, uint8_t type_id, uint8_t cmd)
{
    memset(msg, 0, sizeof(*msg));
    msg->type_id = type_id;
    msg->opcode = cmd;
}

/**
 * @brief Create a simple message with optional payload.
 *
 * @param msg Pointer to message to initialize.
 * @param type_id Type ID byte.
 * @param cmd Command type byte.
 * @param data Pointer to payload data (may be NULL).
 * @param data_len Length of payload (0-27).
 */
static inline void test_msg_create(crumbs_message_t *msg,
                                   uint8_t type_id,
                                   uint8_t cmd,
                                   const uint8_t *data,
                                   uint8_t data_len)
{
    memset(msg, 0, sizeof(*msg));
    msg->type_id = type_id;
    msg->opcode = cmd;
    if (data && data_len > 0 && data_len <= CRUMBS_MAX_PAYLOAD)
    {
        memcpy(msg->data, data, data_len);
        msg->data_len = data_len;
    }
}

/**
 * @brief Encode a message into a buffer and return the length.
 *
 * @param msg Message to encode.
 * @param buf Buffer (should be CRUMBS_MESSAGE_MAX_SIZE).
 * @return Encoded length, or 0 on failure.
 */
static inline size_t test_encode(const crumbs_message_t *msg, uint8_t *buf)
{
    return crumbs_encode_message(msg, buf, CRUMBS_MESSAGE_MAX_SIZE);
}

/**
 * @brief Initialize a peripheral context with standard address 0x10.
 */
static inline void test_init_peripheral(crumbs_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    crumbs_init(ctx, CRUMBS_ROLE_PERIPHERAL, 0x10);
}

/**
 * @brief Initialize a controller context.
 */
static inline void test_init_controller(crumbs_context_t *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    crumbs_init(ctx, CRUMBS_ROLE_CONTROLLER, 0);
}

#endif /* TEST_COMMON_H */
