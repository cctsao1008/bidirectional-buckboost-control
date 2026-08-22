/**
 * @file ring_buffer.h
 * @brief Lock-free single-producer/single-consumer byte ring buffer.
 */

#ifndef BUCKBOOST_RING_BUFFER_H
#define BUCKBOOST_RING_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RING_BUFFER_CAPACITY 256U

typedef struct {
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint32_t overflow_count;
    uint8_t data[RING_BUFFER_CAPACITY];
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t *buffer);
bool ring_buffer_push_isr(ring_buffer_t *buffer, uint8_t byte);
bool ring_buffer_pop(ring_buffer_t *buffer, uint8_t *byte);
size_t ring_buffer_count(const ring_buffer_t *buffer);

#endif /* BUCKBOOST_RING_BUFFER_H */
