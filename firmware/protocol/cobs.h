/**
 * @file cobs.h
 * @brief Consistent Overhead Byte Stuffing codec.
 */

#ifndef BUCKBOOST_COBS_H
#define BUCKBOOST_COBS_H

#include <stddef.h>
#include <stdint.h>

/**
 * Encode a byte buffer using COBS.
 *
 * The returned buffer does not include the trailing 0x00 frame delimiter.
 *
 * @param input Input buffer.
 * @param input_len Number of input bytes.
 * @param output Output buffer.
 * @param output_capacity Size of output buffer in bytes.
 * @return Encoded length, or 0 if the output buffer is too small.
 */
size_t cobs_encode(const uint8_t *input,
                   size_t input_len,
                   uint8_t *output,
                   size_t output_capacity);

/**
 * Decode a COBS-encoded byte buffer.
 *
 * The input must not include the trailing 0x00 frame delimiter.
 *
 * @param input COBS-encoded input buffer.
 * @param input_len Number of encoded bytes.
 * @param output Decoded output buffer.
 * @param output_capacity Size of output buffer in bytes.
 * @return Decoded length, or 0 if the encoded data is invalid or the output
 *         buffer is too small.
 */
size_t cobs_decode(const uint8_t *input,
                   size_t input_len,
                   uint8_t *output,
                   size_t output_capacity);

#endif /* BUCKBOOST_COBS_H */
