/**
 * @file cobs.c
 * @brief Consistent Overhead Byte Stuffing codec implementation.
 */

#include "cobs.h"

size_t cobs_encode(const uint8_t *input,
                   size_t input_len,
                   uint8_t *output,
                   size_t output_capacity)
{
    size_t read_index = 0;
    size_t write_index = 1;
    size_t code_index = 0;
    uint8_t code = 1;

    if ((input_len > 0U && input == NULL) || output == NULL || output_capacity == 0U) {
        return 0U;
    }

    while (read_index < input_len) {
        if (input[read_index] == 0U) {
            if (code_index >= output_capacity) {
                return 0U;
            }
            output[code_index] = code;
            code = 1U;
            code_index = write_index;
            if (write_index >= output_capacity) {
                return 0U;
            }
            ++write_index;
            ++read_index;
            continue;
        }

        if (write_index >= output_capacity) {
            return 0U;
        }
        output[write_index++] = input[read_index++];
        ++code;

        if (code == 0xFFU) {
            if (code_index >= output_capacity) {
                return 0U;
            }
            output[code_index] = code;
            code = 1U;
            code_index = write_index;
            if (write_index >= output_capacity) {
                return 0U;
            }
            ++write_index;
        }
    }

    if (code_index >= output_capacity) {
        return 0U;
    }
    output[code_index] = code;

    return write_index;
}

size_t cobs_decode(const uint8_t *input,
                   size_t input_len,
                   uint8_t *output,
                   size_t output_capacity)
{
    size_t read_index = 0;
    size_t write_index = 0;

    if (input == NULL || output == NULL || input_len == 0U) {
        return 0U;
    }

    while (read_index < input_len) {
        const uint8_t code = input[read_index++];

        if (code == 0U) {
            return 0U;
        }

        for (uint16_t i = 1U; i < code; ++i) {
            if (read_index >= input_len || write_index >= output_capacity) {
                return 0U;
            }
            output[write_index++] = input[read_index++];
        }

        if (code != 0xFFU && read_index < input_len) {
            if (write_index >= output_capacity) {
                return 0U;
            }
            output[write_index++] = 0U;
        }
    }

    return write_index;
}
