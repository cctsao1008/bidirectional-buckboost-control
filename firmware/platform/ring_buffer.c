/**
 * @file ring_buffer.c
 * @brief Lock-free single-producer/single-consumer byte ring buffer.
 */

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *buffer)
{
    if (buffer == NULL) {
        return;
    }

    buffer->head = 0U;
    buffer->tail = 0U;
    buffer->overflow_count = 0U;
}

bool ring_buffer_push_isr(ring_buffer_t *buffer, uint8_t byte)
{
    uint16_t next_head;

    if (buffer == NULL) {
        return false;
    }

    next_head = (uint16_t)((buffer->head + 1U) % RING_BUFFER_CAPACITY);
    if (next_head == buffer->tail) {
        buffer->overflow_count++;
        return false;
    }

    buffer->data[buffer->head] = byte;
    buffer->head = next_head;
    return true;
}

bool ring_buffer_pop(ring_buffer_t *buffer, uint8_t *byte)
{
    if (buffer == NULL || byte == NULL || buffer->tail == buffer->head) {
        return false;
    }

    *byte = buffer->data[buffer->tail];
    buffer->tail = (uint16_t)((buffer->tail + 1U) % RING_BUFFER_CAPACITY);
    return true;
}

size_t ring_buffer_count(const ring_buffer_t *buffer)
{
    if (buffer == NULL) {
        return 0U;
    }

    if (buffer->head >= buffer->tail) {
        return (size_t)(buffer->head - buffer->tail);
    }

    return (size_t)(RING_BUFFER_CAPACITY - buffer->tail + buffer->head);
}
