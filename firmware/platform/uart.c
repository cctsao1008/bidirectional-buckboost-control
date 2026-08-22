/**
 * @file uart.c
 * @brief STM32F334 USART1 transport using libopencm3.
 */

#include "uart.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#include "ring_buffer.h"

static ring_buffer_t rx_buffer;

void platform_uart_init(uint32_t baudrate)
{
    ring_buffer_init(&rx_buffer);

    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_USART1);

    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
    gpio_set_af(GPIOB, GPIO_AF7, GPIO6 | GPIO7);

    usart_disable(USART1);
    usart_set_baudrate(USART1, baudrate);
    usart_set_databits(USART1, 8);
    usart_set_stopbits(USART1, USART_STOPBITS_1);
    usart_set_parity(USART1, USART_PARITY_NONE);
    usart_set_flow_control(USART1, USART_FLOWCONTROL_NONE);
    usart_set_mode(USART1, USART_MODE_TX_RX);

    nvic_enable_irq(NVIC_USART1_EXTI25_IRQ);
    usart_enable_rx_interrupt(USART1);
    usart_enable(USART1);
}

bool platform_uart_read_byte(uint8_t *byte)
{
    return ring_buffer_pop(&rx_buffer, byte);
}

void platform_uart_write(const uint8_t *data, size_t length)
{
    if (data == NULL) {
        return;
    }

    for (size_t i = 0U; i < length; ++i) {
        usart_send_blocking(USART1, data[i]);
    }
}

uint32_t platform_uart_rx_overflow_count(void)
{
    return rx_buffer.overflow_count;
}

void usart1_exti25_isr(void)
{
    while (usart_get_flag(USART1, USART_FLAG_RXNE)) {
        const uint8_t byte = (uint8_t)usart_recv(USART1);
        (void)ring_buffer_push_isr(&rx_buffer, byte);
    }
}
