/**
 * @file board_io.c
 * @brief Safe initialization for non-power-stage board GPIOs.
 */

#include "board_io.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define ADC_GPIO              GPIOA
#define ADC_GPIO_RCC          RCC_GPIOA
#define ADC_INPUT_PINS        (GPIO0 | GPIO1 | GPIO2 | GPIO3 | GPIO4)
#define UNUSED_GPIOA_PINS     (GPIO5 | GPIO6 | GPIO7 | GPIO12 | GPIO15)

#define BOARD_GPIO            GPIOB
#define BOARD_GPIO_RCC        RCC_GPIOB
#define STATUS_LED_PINS       (GPIO0 | GPIO1 | GPIO2)
#define KEY_PINS              (GPIO3 | GPIO4)
#define KEY1_PIN              GPIO3
#define KEY2_PIN              GPIO4
#define UART_IDLE_PINS        (GPIO6 | GPIO7)
#define OLED_I2C_PINS         (GPIO8 | GPIO9)
#define UNUSED_GPIOB_PINS     (GPIO5 | GPIO10 | GPIO11 | GPIO12 | GPIO13 | GPIO14 | GPIO15)

#define UNUSED_GPIOC          GPIOC
#define UNUSED_GPIOC_RCC      RCC_GPIOC
#define UNUSED_GPIOC_PINS     (GPIO13 | GPIO14 | GPIO15)

#define UNUSED_GPIOF          GPIOF
#define UNUSED_GPIOF_RCC      RCC_GPIOF
#define UNUSED_GPIOF_PINS     (GPIO0 | GPIO1)

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
    /* PB0/PB1/PB2 = green/yellow/red. All three are active-high. */
    gpio_clear(BOARD_GPIO, STATUS_LED_PINS);
}

static void init_status_leds_off(void)
{
    /* Preload LOW before enabling the pins as outputs. */
    platform_status_leds_all_off();

    gpio_mode_setup(BOARD_GPIO,
                    GPIO_MODE_OUTPUT,
                    GPIO_PUPD_NONE,
                    STATUS_LED_PINS);
    gpio_set_output_options(BOARD_GPIO,
                            GPIO_OTYPE_PP,
                            GPIO_OSPEED_2MHZ,
                            STATUS_LED_PINS);

    platform_status_leds_all_off();
}

static void init_key_inputs(void)
{
    /*
     * V1.2 board mapping:
     *   PB3 -> KEY1
     *   PB4 -> KEY2
     *
     * Both inputs already have external 10 kOhm pull-ups to 3.3 V and RC
     * filtering on the board. A pressed key pulls the corresponding input low,
     * so no additional MCU pull resistor is enabled here.
     */
    gpio_mode_setup(BOARD_GPIO,
                    GPIO_MODE_INPUT,
                    GPIO_PUPD_NONE,
                    KEY_PINS);
}

bool platform_key1_pressed(void)
{
    return gpio_get(BOARD_GPIO, KEY1_PIN) == 0U;
}

bool platform_key2_pressed(void)
{
    return gpio_get(BOARD_GPIO, KEY2_PIN) == 0U;
}

static void init_uart_pins_idle(void)
{
    /*
     * PB6/PB7 are USART1_TX/USART1_RX. Keep both as passive pulled-up inputs
     * until uart.c switches ownership to USART1 alternate-function mode.
     * UART idle is logic-high, so this also avoids a floating startup state.
     */
    gpio_mode_setup(BOARD_GPIO,
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
    gpio_mode_setup(BOARD_GPIO,
                    GPIO_MODE_INPUT,
                    GPIO_PUPD_NONE,
                    OLED_I2C_PINS);
}

static void init_verified_unused_pins_analog(void)
{
    /*
     * The following pins are unconnected in the CBB024D V1.2 schematic.
     * Put them in analog mode to avoid floating digital inputs and to make
     * their startup state deterministic.
     *
     * PA13/PA14 are deliberately excluded because SWD must remain available.
     * PA8..PA11 are deliberately excluded because power_stage.c owns them.
     */
    gpio_mode_setup(GPIOA,
                    GPIO_MODE_ANALOG,
                    GPIO_PUPD_NONE,
                    UNUSED_GPIOA_PINS);
    gpio_mode_setup(GPIOB,
                    GPIO_MODE_ANALOG,
                    GPIO_PUPD_NONE,
                    UNUSED_GPIOB_PINS);
    gpio_mode_setup(UNUSED_GPIOC,
                    GPIO_MODE_ANALOG,
                    GPIO_PUPD_NONE,
                    UNUSED_GPIOC_PINS);
    gpio_mode_setup(UNUSED_GPIOF,
                    GPIO_MODE_ANALOG,
                    GPIO_PUPD_NONE,
                    UNUSED_GPIOF_PINS);
}

void platform_board_io_init_safe(void)
{
    rcc_periph_clock_enable(ADC_GPIO_RCC);
    rcc_periph_clock_enable(BOARD_GPIO_RCC);
    rcc_periph_clock_enable(UNUSED_GPIOC_RCC);
    rcc_periph_clock_enable(UNUSED_GPIOF_RCC);

    /* Initialize verified non-power-stage GPIO groups in safety order. */
    init_adc_pins_analog();
    init_status_leds_off();
    init_key_inputs();
    init_uart_pins_idle();
    init_oled_bus_idle();
    init_verified_unused_pins_analog();
}
