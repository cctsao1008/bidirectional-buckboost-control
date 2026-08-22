/**
 * @file clock.c
 * @brief STM32F334 system-clock initialization using libopencm3.
 */

#include "clock.h"

#include <libopencm3/stm32/rcc.h>

void platform_clock_init(void)
{
    rcc_clock_setup_pll(&rcc_hsi_configs[RCC_CLOCK_HSI_64MHZ]);
}
