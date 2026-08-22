/**
 * @file uart.h
 * @brief STM32F334 USART1 transport interface for host communication.
 */

#ifndef BUCKBOOST_UART_H
#define BUCKBOOST_UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Initialize USART1 on PB6/PB7 for host communication. */
void platform_uart_init(uint32_t baudrate);

/** Pop one received byte from the interrupt-fed RX ring buffer. */
bool platform_uart_read_byte(uint8_t *byte);

/** Write bytes synchronously from background context. */
void platform_uart_write(const uint8_t *data, size_t length);

/** Return the number of bytes dropped because the RX ring buffer was full. */
uint32_t platform_uart_rx_overflow_count(void);

#endif /* BUCKBOOST_UART_H */
