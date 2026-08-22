/**
 * @file board_io.c
 * @brief Safe initialization for non-power-stage board GPIOs.
 */

#include "board_io.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define ADC_GPIO            GPIOA
#define ADC_GPIO_RCC        RCC_GPIOA
#define ADC_INPUT_PINS      (GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4)

#define STATUS_GPIO         GPIOB
#define STATUS_GPIO_RCC     RCC_GPIOB
#define STATUS_LED_PINS     (GPIO0 | GPIO1 | GPIO2)
#define UART_IDLE_PINS      (GPIO6 | GPIO7)
#define OLED_I2C_PINS       (GPIO8 | GPIO9)

static void init_adc_pins_analog(void)
{
    /*
     * V1.2 board mapping:
     *   PA0 -> ADC_Vin
     *   PA1 -> ADC_Iin
     *   PA2 -> ADC_Vout
     *   PA3 -> ADC_Iout
     *   PA4 -> ADC_VADJ
     *
     * Analog mode disables the digital input path and is the deterministic
     * startup state required before the ADC peripheral is configured.
     */
    gpio_mode_setup(ADC_GPIO,
                    GPIO_MODE_ANALOG,
                    GPIO_PUPD_NONE,
                    ADC_INPUT_PINS);
}

void platform_status_leds_all_off(void)
{
    /* The V1.2 status LEDs are active-high. */
    gpio_clear(STATUS_GPIO, STATUS_LED_PINS);
}

static void init_status_leds_off(void)
{
    /* Preload LOW before enabling the pins as outputs. */
    platform_status_leds_all_off();

    gpio_mode_setup(STATUS_GPIO,
                    GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE,
                    STATUS_LED_PINS);
    gpio_set_output_options(STATUS_GPIO,
                            GPIO_OTYPE_PP,
                            GPIO_OSPEED_2MHZ,
                            STATUS_LED_PINS);

    platform_status_leds_all_off();
}

static void init_uart_pins_idle(void)
{
    /*
     * PB6/PB7 are USART1_TX/USART1_RX. Keep both as passive pulled-up inputs
     * until uart.c switches ownership to USART1 alternate function mode.
     * UART idle is logic-high, so this also avoids a floating startup state.
     */
    gpio_mode_setup(STATUS_GPIO,
                    GPIO_MODE_INPUT,
                    GPIO_PUPD_PULLUP,
                    UART_IDLE_PINS);
}

static void init_oled_bus_idle(void)
{
    /*
     * PB8/PB9 are I2C1_SCL/I2C1_SDA. Leave them high-impedance until the I2C
     * driver is implemented. This avoids actively driving the external OLED
     * module or its bus during Gate 1 bring-up.
     */
    gpio_mode_setup(STATUS_GPIO,
                    GPIO_MODE_INPUT,
                    GPIO_PUPD_NONE,
                    OLED_I2C_PINS);
}

void platform_board_io_init_safe(void)
{
    rcc_periph_clock_enable(ADC_GPIO_RCC);
    rcc_periph_clock_enable(STATUS_GPIO_RCC);

    /* Initialize verified non-power-stage GPIO groups in safety order. */
    init_adc_pins_analog();
    init_status_leds_off();
    init_uart_pins_idle();
    init_oled_bus_idle();
}
