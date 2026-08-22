/**
 * @file protocol.h
 * @brief Versioned COBS-framed host protocol definitions and codec API.
 */

#ifndef BUCKBOOST_PROTOCOL_H
#define BUCKBOOST_PROTOCOL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PROTOCOL_VERSION_MAJOR       1U
#define PROTOCOL_MAX_PAYLOAD         240U
#define PROTOCOL_RAW_HEADER_SIZE     8U
#define PROTOCOL_CRC_SIZE            2U
#define PROTOCOL_MAX_RAW_SIZE        (PROTOCOL_RAW_HEADER_SIZE + PROTOCOL_MAX_PAYLOAD + PROTOCOL_CRC_SIZE)
#define PROTOCOL_MAX_ENCODED_SIZE    (PROTOCOL_MAX_RAW_SIZE + (PROTOCOL_MAX_RAW_SIZE / 254U) + 2U)

#define PROTOCOL_FRAME_DELIMITER     0x00U

typedef enum {
    PROTOCOL_TYPE_REQUEST = 0x01,
    PROTOCOL_TYPE_RESPONSE = 0x02,
    PROTOCOL_TYPE_EVENT = 0x03,
    PROTOCOL_TYPE_TELEMETRY = 0x04,
} protocol_type_t;

typedef enum {
    PROTOCOL_CMD_PING = 0x01,
    PROTOCOL_CMD_GET_INFO = 0x02,
    PROTOCOL_CMD_GET_STATUS = 0x03,
    PROTOCOL_CMD_SET_VREF = 0x10,
    PROTOCOL_CMD_SET_ILIMIT = 0x11,
    PROTOCOL_CMD_OUTPUT_ENABLE = 0x12,
    PROTOCOL_CMD_OUTPUT_DISABLE = 0x13,
    PROTOCOL_CMD_CLEAR_FAULT = 0x14,
} protocol_command_t;

typedef enum {
    PROTOCOL_STATUS_OK = 0x00,
    PROTOCOL_STATUS_ERR_BAD_CMD = 0x01,
    PROTOCOL_STATUS_ERR_BAD_LENGTH = 0x02,
    PROTOCOL_STATUS_ERR_BAD_VALUE = 0x03,
    PROTOCOL_STATUS_ERR_BAD_STATE = 0x04,
    PROTOCOL_STATUS_ERR_FAULT_ACTIVE = 0x05,
    PROTOCOL_STATUS_ERR_BUSY = 0x06,
    PROTOCOL_STATUS_ERR_INTERNAL = 0x07,
} protocol_status_t;

typedef enum {
    PROTOCOL_RESULT_OK = 0,
    PROTOCOL_RESULT_NO_FRAME,
    PROTOCOL_RESULT_BAD_ARGUMENT,
    PROTOCOL_RESULT_BUFFER_TOO_SMALL,
    PROTOCOL_RESULT_COBS_ERROR,
    PROTOCOL_RESULT_BAD_VERSION,
    PROTOCOL_RESULT_BAD_LENGTH,
    PROTOCOL_RESULT_BAD_CRC,
    PROTOCOL_RESULT_STREAM_OVERFLOW,
} protocol_result_t;

typedef struct {
    uint8_t version;
    uint8_t type;
    uint8_t command;
    uint8_t flags;
    uint16_t sequence;
    uint16_t payload_length;
    uint8_t payload[PROTOCOL_MAX_PAYLOAD];
} protocol_frame_t;

typedef struct {
    uint8_t encoded[PROTOCOL_MAX_ENCODED_SIZE];
    size_t encoded_length;
    bool dropping_until_delimiter;
    uint32_t frames_ok;
    uint32_t cobs_errors;
    uint32_t crc_errors;
    uint32_t length_errors;
    uint32_t version_errors;
    uint32_t overflow_errors;
} protocol_stream_t;

/** Encode a protocol frame as COBS data followed by a 0x00 delimiter. */
protocol_result_t protocol_encode_frame(const protocol_frame_t *frame,
                                        uint8_t *output,
                                        size_t output_capacity,
                                        size_t *output_length);

/** Decode a single COBS frame. Input must not include the trailing delimiter. */
protocol_result_t protocol_decode_frame(const uint8_t *encoded,
                                        size_t encoded_length,
                                        protocol_frame_t *frame);

/** Initialize a streaming UART frame receiver. */
void protocol_stream_init(protocol_stream_t *stream);

/**
 * Push one UART byte into the stream decoder.
 *
 * Returns PROTOCOL_RESULT_OK when a complete valid frame is written to
 * frame_out. PROTOCOL_RESULT_NO_FRAME means more bytes are required.
 */
protocol_result_t protocol_stream_push(protocol_stream_t *stream,
                                       uint8_t byte,
                                       protocol_frame_t *frame_out);

#endif /* BUCKBOOST_PROTOCOL_H */
