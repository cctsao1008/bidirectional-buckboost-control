/**
 * @file protocol.c
 * @brief Versioned COBS-framed host protocol implementation.
 */

#include "protocol.h"

#include <string.h>

#include "cobs.h"
#include "crc16.h"

static void write_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static uint16_t read_u16_le(const uint8_t *src)
{
    return (uint16_t)src[0] | ((uint16_t)src[1] << 8U);
}

protocol_result_t protocol_encode_frame(const protocol_frame_t *frame,
                                        uint8_t *output,
                                        size_t output_capacity,
                                        size_t *output_length)
{
    uint8_t raw[PROTOCOL_MAX_RAW_SIZE];
    size_t raw_length;
    size_t encoded_length;
    uint16_t crc;

    if (frame == NULL || output == NULL || output_length == NULL) {
        return PROTOCOL_RESULT_BAD_ARGUMENT;
    }

    if (frame->payload_length > PROTOCOL_MAX_PAYLOAD) {
        return PROTOCOL_RESULT_BAD_LENGTH;
    }

    raw[0] = frame->version;
    raw[1] = frame->type;
    raw[2] = frame->command;
    raw[3] = frame->flags;
    write_u16_le(&raw[4], frame->sequence);
    write_u16_le(&raw[6], frame->payload_length);

    if (frame->payload_length > 0U) {
        memcpy(&raw[PROTOCOL_RAW_HEADER_SIZE], frame->payload, frame->payload_length);
    }

    raw_length = PROTOCOL_RAW_HEADER_SIZE + frame->payload_length;
    crc = crc16_ccitt_false(raw, raw_length);
    write_u16_le(&raw[raw_length], crc);
    raw_length += PROTOCOL_CRC_SIZE;

    if (output_capacity < 2U) {
        return PROTOCOL_RESULT_BUFFER_TOO_SMALL;
    }

    encoded_length = cobs_encode(raw, raw_length, output, output_capacity - 1U);
    if (encoded_length == 0U || encoded_length >= output_capacity) {
        return PROTOCOL_RESULT_BUFFER_TOO_SMALL;
    }

    output[encoded_length++] = PROTOCOL_FRAME_DELIMITER;
    *output_length = encoded_length;

    return PROTOCOL_RESULT_OK;
}

protocol_result_t protocol_decode_frame(const uint8_t *encoded,
                                        size_t encoded_length,
                                        protocol_frame_t *frame)
{
    uint8_t raw[PROTOCOL_MAX_RAW_SIZE];
    size_t raw_length;
    size_t expected_length;
    uint16_t payload_length;
    uint16_t received_crc;
    uint16_t calculated_crc;

    if (encoded == NULL || frame == NULL || encoded_length == 0U) {
        return PROTOCOL_RESULT_BAD_ARGUMENT;
    }

    raw_length = cobs_decode(encoded, encoded_length, raw, sizeof(raw));
    if (raw_length == 0U) {
        return PROTOCOL_RESULT_COBS_ERROR;
    }

    if (raw_length < PROTOCOL_RAW_HEADER_SIZE + PROTOCOL_CRC_SIZE) {
        return PROTOCOL_RESULT_BAD_LENGTH;
    }

    if (raw[0] != PROTOCOL_VERSION_MAJOR) {
        return PROTOCOL_RESULT_BAD_VERSION;
    }

    payload_length = read_u16_le(&raw[6]);
    if (payload_length > PROTOCOL_MAX_PAYLOAD) {
        return PROTOCOL_RESULT_BAD_LENGTH;
    }

    expected_length = PROTOCOL_RAW_HEADER_SIZE + payload_length + PROTOCOL_CRC_SIZE;
    if (raw_length != expected_length) {
        return PROTOCOL_RESULT_BAD_LENGTH;
    }

    received_crc = read_u16_le(&raw[raw_length - PROTOCOL_CRC_SIZE]);
    calculated_crc = crc16_ccitt_false(raw, raw_length - PROTOCOL_CRC_SIZE);
    if (received_crc != calculated_crc) {
        return PROTOCOL_RESULT_BAD_CRC;
    }

    frame->version = raw[0];
    frame->type = raw[1];
    frame->command = raw[2];
    frame->flags = raw[3];
    frame->sequence = read_u16_le(&raw[4]);
    frame->payload_length = payload_length;

    if (payload_length > 0U) {
        memcpy(frame->payload, &raw[PROTOCOL_RAW_HEADER_SIZE], payload_length);
    }

    return PROTOCOL_RESULT_OK;
}

void protocol_stream_init(protocol_stream_t *stream)
{
    if (stream == NULL) {
        return;
    }

    memset(stream, 0, sizeof(*stream));
}

protocol_result_t protocol_stream_push(protocol_stream_t *stream,
                                       uint8_t byte,
                                       protocol_frame_t *frame_out)
{
    protocol_result_t result;

    if (stream == NULL || frame_out == NULL) {
        return PROTOCOL_RESULT_BAD_ARGUMENT;
    }

    if (byte != PROTOCOL_FRAME_DELIMITER) {
        if (stream->dropping_until_delimiter) {
            return PROTOCOL_RESULT_NO_FRAME;
        }

        if (stream->encoded_length >= sizeof(stream->encoded)) {
            stream->dropping_until_delimiter = true;
            stream->overflow_errors++;
            return PROTOCOL_RESULT_STREAM_OVERFLOW;
        }

        stream->encoded[stream->encoded_length++] = byte;
        return PROTOCOL_RESULT_NO_FRAME;
    }

    if (stream->dropping_until_delimiter) {
        stream->dropping_until_delimiter = false;
        stream->encoded_length = 0U;
        return PROTOCOL_RESULT_STREAM_OVERFLOW;
    }

    if (stream->encoded_length == 0U) {
        return PROTOCOL_RESULT_NO_FRAME;
    }

    result = protocol_decode_frame(stream->encoded, stream->encoded_length, frame_out);
    stream->encoded_length = 0U;

    switch (result) {
    case PROTOCOL_RESULT_OK:
        stream->frames_ok++;
        break;
    case PROTOCOL_RESULT_COBS_ERROR:
        stream->cobs_errors++;
        break;
    case PROTOCOL_RESULT_BAD_CRC:
        stream->crc_errors++;
        break;
    case PROTOCOL_RESULT_BAD_LENGTH:
        stream->length_errors++;
        break;
    case PROTOCOL_RESULT_BAD_VERSION:
        stream->version_errors++;
        break;
    default:
        break;
    }

    return result;
}
