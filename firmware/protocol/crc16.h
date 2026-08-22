/**
 * @file crc16.h
 * @brief CRC-16/CCITT-FALSE calculation.
 */

#ifndef BUCKBOOST_CRC16_H
#define BUCKBOOST_CRC16_H

#include <stddef.h>
#include <stdint.h>

/**
 * Calculate CRC-16/CCITT-FALSE.
 *
 * Parameters:
 * - polynomial: 0x1021
 * - initial value: 0xFFFF
 * - refin/refout: false
 * - xorout: 0x0000
 *
 * @param data Input bytes.
 * @param length Number of input bytes.
 * @return Calculated CRC value.
 */
uint16_t crc16_ccitt_false(const uint8_t *data, size_t length);

#endif /* BUCKBOOST_CRC16_H */
