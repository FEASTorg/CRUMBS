/**
 * \file
 * Functions and types for CRC checks.
 *
 * Generated on Sun Nov 23 21:32:19 2025
 * by pycrc v0.11.0, https://pycrc.org
 * using the configuration:
 *  - Width         = 8
 *  - Poly          = 0x07
 *  - XorIn         = 0x00
 *  - ReflectIn     = False
 *  - XorOut        = 0x00
 *  - ReflectOut    = False
 *  - Algorithm     = table-driven
 *
 * This file defines the functions crc8_nibble_init(), crc8_nibble_update() and crc8_nibble_finalize().
 *
 * The crc8_nibble_init() function returns the initial \c crc value and must be called
 * before the first call to crc8_nibble_update().
 * Similarly, the crc8_nibble_finalize() function must be called after the last call
 * to crc8_nibble_update(), before the \c crc is being used.
 * is being used.
 *
 * The crc8_nibble_update() function can be called any number of times (including zero
 * times) in between the crc8_nibble_init() and crc8_nibble_finalize() calls.
 *
 * This pseudo-code shows an example usage of the API:
 * \code{.c}
 * crc8_nibble_t crc;
 * unsigned char data[MAX_DATA_LEN];
 * size_t data_len;
 *
 * crc = crc8_nibble_init();
 * while ((data_len = read_data(data, MAX_DATA_LEN)) > 0) {
 *     crc = crc8_nibble_update(crc, data, data_len);
 * }
 * crc = crc8_nibble_finalize(crc);
 * \endcode
 * Auto converted to Arduino C++ on Sun Nov 23 21:41:49 2025
 * DO NOT EDIT
 */
#ifndef CRC8_NIBBLE_HPP
#define CRC8_NIBBLE_HPP

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * The definition of the used algorithm.
 *
 * This is not used anywhere in the generated code, but it may be used by the
 * application code to call algorithm-specific code, if desired.
 */
const uint8_t CRC8_NIBBLE_ALGO_TABLE_DRIVEN = 1;


/**
 * The type of the CRC values.
 *
 * This type must be big enough to contain at least 8 bits.
 */
typedef uint8_t crc8_nibble_t;


/**
 * Calculate the initial crc value.
 *
 * \return     The initial crc value.
 */
inline crc8_nibble_t crc8_nibble_init(void)
{
    return 0x00;
}


/**
 * Update the crc value with new data.
 *
 * \param[in] crc      The current crc value.
 * \param[in] data     Pointer to a buffer of \a data_len bytes.
 * \param[in] data_len Number of bytes in the \a data buffer.
 * \return             The updated crc value.
 */
crc8_nibble_t crc8_nibble_update(crc8_nibble_t crc, const void *data, size_t data_len);


/**
 * Calculate the final crc value.
 *
 * \param[in] crc  The current crc value.
 * \return     The final crc value.
 */
inline crc8_nibble_t crc8_nibble_finalize(crc8_nibble_t crc)
{
    return crc;
}


/**
 * Calculate the crc in one-shot.
 * Convenience helper exposed in the global namespace.
 *
 * \param[in] data     Pointer to a buffer of \a data_len bytes.
 * \param[in] data_len Number of bytes in the \a data buffer.
 */
inline crc8_nibble_t crc8_nibble_calculate(const void *data, size_t data_len) {
  crc8_nibble_t crc = crc8_nibble_init();
  crc = crc8_nibble_update(crc, data, data_len);
  return crc8_nibble_finalize(crc);
}
#ifdef __cplusplus
}           /* closing brace for extern "C" */
#endif

#endif      /* CRC8_NIBBLE_HPP */
