/**
 * @file test_protocol.c
 * @brief Host-side unit tests for COBS, CRC16, and protocol framing.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../firmware/protocol/cobs.h"
#include "../../firmware/protocol/crc16.h"
#include "../../firmware/protocol/protocol.h"

static void test_crc16_known_vector(void)
{
    static const uint8_t data[] = "123456789";
    assert(crc16_ccitt_false(data, 9U) == 0x29B1U);
}

static void test_cobs_round_trip(void)
{
    static const uint8_t input[] = {
        0x11U, 0x22U, 0x00U, 0x33U, 0x00U, 0x00U, 0x44U, 0x55U
    };
    uint8_t encoded[32];
    uint8_t decoded[32];
    size_t encoded_length;
    size_t decoded_length;

    encoded_length = cobs_encode(input, sizeof(input), encoded, sizeof(encoded));
    assert(encoded_length > 0U);

    for (size_t i = 0U; i < encoded_length; ++i) {
        assert(encoded[i] != 0U);
    }

    decoded_length = cobs_decode(encoded, encoded_length, decoded, sizeof(decoded));
    assert(decoded_length == sizeof(input));
    assert(memcmp(input, decoded, sizeof(input)) == 0);
}

static void test_protocol_round_trip(void)
{
    protocol_frame_t tx = {0};
    protocol_frame_t rx = {0};
    uint8_t encoded[PROTOCOL_MAX_ENCODED_SIZE];
    size_t encoded_length = 0U;

    tx.version = PROTOCOL_VERSION_MAJOR;
    tx.type = PROTOCOL_TYPE_REQUEST;
    tx.command = PROTOCOL_CMD_SET_VREF;
    tx.flags = 0U;
    tx.sequence = 0x1234U;
    tx.payload_length = 4U;
    tx.payload[0] = 0x50U;
    tx.payload[1] = 0x46U;
    tx.payload[2] = 0x00U;
    tx.payload[3] = 0x00U;

    assert(protocol_encode_frame(&tx, encoded, sizeof(encoded), &encoded_length) == PROTOCOL_RESULT_OK);
    assert(encoded_length > 1U);
    assert(encoded[encoded_length - 1U] == PROTOCOL_FRAME_DELIMITER);

    assert(protocol_decode_frame(encoded, encoded_length - 1U, &rx) == PROTOCOL_RESULT_OK);
    assert(rx.version == tx.version);
    assert(rx.type == tx.type);
    assert(rx.command == tx.command);
    assert(rx.flags == tx.flags);
    assert(rx.sequence == tx.sequence);
    assert(rx.payload_length == tx.payload_length);
    assert(memcmp(rx.payload, tx.payload, tx.payload_length) == 0);
}

static void test_stream_resynchronization(void)
{
    protocol_stream_t stream;
    protocol_frame_t tx = {0};
    protocol_frame_t rx = {0};
    uint8_t encoded[PROTOCOL_MAX_ENCODED_SIZE];
    size_t encoded_length = 0U;
    protocol_result_t result = PROTOCOL_RESULT_NO_FRAME;

    protocol_stream_init(&stream);

    tx.version = PROTOCOL_VERSION_MAJOR;
    tx.type = PROTOCOL_TYPE_REQUEST;
    tx.command = PROTOCOL_CMD_PING;
    tx.sequence = 7U;
    tx.payload_length = 0U;

    assert(protocol_encode_frame(&tx, encoded, sizeof(encoded), &encoded_length) == PROTOCOL_RESULT_OK);

    /* Garbage frame first: delimiter must provide a clean resynchronization point. */
    assert(protocol_stream_push(&stream, 0xAAU, &rx) == PROTOCOL_RESULT_NO_FRAME);
    assert(protocol_stream_push(&stream, 0xBBU, &rx) == PROTOCOL_RESULT_NO_FRAME);
    (void)protocol_stream_push(&stream, PROTOCOL_FRAME_DELIMITER, &rx);

    for (size_t i = 0U; i < encoded_length; ++i) {
        result = protocol_stream_push(&stream, encoded[i], &rx);
    }

    assert(result == PROTOCOL_RESULT_OK);
    assert(rx.command == PROTOCOL_CMD_PING);
    assert(rx.sequence == 7U);
    assert(stream.frames_ok == 1U);
}

int main(void)
{
    test_crc16_known_vector();
    test_cobs_round_trip();
    test_protocol_round_trip();
    test_stream_resynchronization();

    puts("protocol tests: PASS");
    return 0;
}
