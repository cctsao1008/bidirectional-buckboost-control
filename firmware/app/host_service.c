/**
 * @file host_service.c
 * @brief Background host command service for Gate 1 communication bring-up.
 */

#include "host_service.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "protocol.h"
#include "timebase.h"
#include "uart.h"
#include "version.h"

#define PROTOCOL_VERSION_MINOR 0U
#define CONTROLLER_CAPABILITIES 0U

static protocol_stream_t rx_stream;
static protocol_frame_t rx_frame;
static protocol_frame_t tx_frame;
static uint8_t tx_encoded[PROTOCOL_MAX_ENCODED_SIZE];

static void write_u16_le(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8U) & 0xFFU);
}

static void write_u32_le(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value & 0xFFU);
    dst[1] = (uint8_t)((value >> 8U) & 0xFFU);
    dst[2] = (uint8_t)((value >> 16U) & 0xFFU);
    dst[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

static void send_response(const protocol_frame_t *request,
                          protocol_status_t status,
                          const uint8_t *payload,
                          uint16_t payload_length)
{
    size_t encoded_length = 0U;

    memset(&tx_frame, 0, sizeof(tx_frame));
    tx_frame.version = PROTOCOL_VERSION_MAJOR;
    tx_frame.type = PROTOCOL_TYPE_RESPONSE;
    tx_frame.command = request->command;
    tx_frame.flags = 0U;
    tx_frame.sequence = request->sequence;
    tx_frame.payload[0] = (uint8_t)status;

    if (payload_length > 0U && payload != NULL) {
        memcpy(&tx_frame.payload[1], payload, payload_length);
    }

    tx_frame.payload_length = (uint16_t)(payload_length + 1U);

    if (protocol_encode_frame(&tx_frame,
                              tx_encoded,
                              sizeof(tx_encoded),
                              &encoded_length) == PROTOCOL_RESULT_OK) {
        platform_uart_write(tx_encoded, encoded_length);
    }
}

static void handle_ping(const protocol_frame_t *request)
{
    uint8_t payload[4];

    if (request->payload_length != 0U) {
        send_response(request, PROTOCOL_STATUS_ERR_BAD_LENGTH, NULL, 0U);
        return;
    }

    write_u32_le(payload, platform_uptime_ms());
    send_response(request, PROTOCOL_STATUS_OK, payload, sizeof(payload));
}

static void handle_get_info(const protocol_frame_t *request)
{
    uint8_t payload[10];

    if (request->payload_length != 0U) {
        send_response(request, PROTOCOL_STATUS_ERR_BAD_LENGTH, NULL, 0U);
        return;
    }

    payload[0] = PROTOCOL_VERSION_MAJOR;
    payload[1] = PROTOCOL_VERSION_MINOR;
    payload[2] = FIRMWARE_VERSION_MAJOR;
    payload[3] = FIRMWARE_VERSION_MINOR;
    payload[4] = FIRMWARE_VERSION_PATCH;
    payload[5] = CONTROLLER_CAPABILITIES;
    write_u32_le(&payload[6], FIRMWARE_BUILD_ID);

    send_response(request, PROTOCOL_STATUS_OK, payload, sizeof(payload));
}

static void handle_get_status(const protocol_frame_t *request)
{
    uint8_t payload[40];
    size_t offset = 0U;

    if (request->payload_length != 0U) {
        send_response(request, PROTOCOL_STATUS_ERR_BAD_LENGTH, NULL, 0U);
        return;
    }

    /* Gate 1 deliberately reports a safe, inactive converter state. */
    write_u32_le(&payload[offset], platform_uptime_ms());
    offset += 4U;

    payload[offset++] = 0U; /* power_state: OFF */
    payload[offset++] = 0U; /* operating_region: UNKNOWN */
    payload[offset++] = 0U; /* controller_type: NONE */
    payload[offset++] = 0U; /* status_flags */

    write_u32_le(&payload[offset], 0U); /* vin_mV */
    offset += 4U;
    write_u32_le(&payload[offset], 0U); /* iin_mA */
    offset += 4U;
    write_u32_le(&payload[offset], 0U); /* vout_mV */
    offset += 4U;
    write_u32_le(&payload[offset], 0U); /* iout_mA */
    offset += 4U;
    write_u32_le(&payload[offset], 0U); /* vref_mV */
    offset += 4U;
    write_u32_le(&payload[offset], 0U); /* ilimit_mA */
    offset += 4U;
    write_u32_le(&payload[offset], 0U); /* fault_bits */
    offset += 4U;

    write_u16_le(&payload[offset], 0U); /* duty_a_q15 */
    offset += 2U;
    write_u16_le(&payload[offset], 0U); /* duty_b_q15 */
    offset += 2U;

    send_response(request, PROTOCOL_STATUS_OK, payload, (uint16_t)offset);
}

static void dispatch_request(const protocol_frame_t *request)
{
    if (request->type != PROTOCOL_TYPE_REQUEST) {
        return;
    }

    switch ((protocol_command_t)request->command) {
    case PROTOCOL_CMD_PING:
        handle_ping(request);
        break;
    case PROTOCOL_CMD_GET_INFO:
        handle_get_info(request);
        break;
    case PROTOCOL_CMD_GET_STATUS:
        handle_get_status(request);
        break;
    default:
        /* Gate 1 exposes no command capable of changing power-stage state. */
        send_response(request, PROTOCOL_STATUS_ERR_BAD_CMD, NULL, 0U);
        break;
    }
}

void host_service_init(void)
{
    protocol_stream_init(&rx_stream);
}

void host_service_run(void)
{
    uint8_t byte;

    while (platform_uart_read_byte(&byte)) {
        const protocol_result_t result = protocol_stream_push(&rx_stream, byte, &rx_frame);

        if (result == PROTOCOL_RESULT_OK) {
            dispatch_request(&rx_frame);
        }
    }
}
