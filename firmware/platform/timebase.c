/**
 * @file timebase.c
 * @brief Millisecond SysTick timebase using libopencm3.
 */

#include "timebase.h"

#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>

static volatile uint32_t uptime_ms;

void platform_timebase_init(void)
{
    uptime_ms = 0U;

    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    (void)systick_set_frequency(1000U, rcc_ahb_frequency);
    systick_clear();
    systick_interrupt_enable();
    systick_counter_enable();
}

uint32_t platform_uptime_ms(void)
{
    return uptime_ms;
}

void sys_tick_handler(void)
{
    ++uptime_ms;
}
