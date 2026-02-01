/**
 * @file lhwit_ops.h
 * @brief Convenience header for all LHWIT family operation definitions
 *
 * This header includes all operation definitions for the LHWIT
 * (Low Hardware Implementation Test) family, which consists of:
 * - LED Controller (Type 0x01)
 * - Servo Controller (Type 0x02)
 * - Calculator (Type 0x03)
 *
 * Include this single header in your controller applications to access
 * all operations for the LHWIT family.
 */

#ifndef LHWIT_OPS_H
#define LHWIT_OPS_H

#include "led_ops.h"
#include "servo_ops.h"
#include "calculator_ops.h"

#endif /* LHWIT_OPS_H */
