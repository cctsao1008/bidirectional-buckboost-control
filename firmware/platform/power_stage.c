/**
 * @file power_stage.c
 * @brief Safe startup control for the converter gate-drive inputs.
 */

#include "power_stage.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#define POWER_STAGE_GPIO       GPIOA
#define POWER_STAGE_GPIO_RCC   RCC_GPIOA
#define POWER_STAGE_GATE_PINS  (GPIO8 | GPIO9 | GPIO10 | GPIO11)

void platform_power_stage_force_off(void)
{
    gpio_clear(POWER_STAGE_GPIO, POWER_STAGE_GATE_PINS);
}

void platform_power_stage_init_safe(void)
{
    rcc_periph_clock_enable(POWER_STAGE_GPIO_RCC);

    /*
     * Preload the output latch low before changing the pins from their reset
     * state to push-pull outputs. This avoids intentionally driving any
     * gate-driver input high during firmware startup.
     *
     * V1.2 board mapping:
     *   PA8  PWM1H -> Q1 high-side
     *   PA9  PWM1L -> Q4 low-side
     *   PA10 PWM2H -> Q2 high-side
     *   PA11 PWM2L -> Q3 low-side
     */
    platform_power_stage_force_off();

    gpio_mode_setup(POWER_STAGE_GPIO,
                    GPIO_MODE_OUTPUT,
                    GPIO_PUPD_PULLDOWN,
                    POWER_STAGE_GATE_PINS);
    gpio_set_output_options(POWER_STAGE_GPIO,
                            GPIO_OTYPE_PP,
                            GPIO_OSPEED_2MHZ,
                            POWER_STAGE_GATE_PINS);

    platform_power_stage_force_off();
}
