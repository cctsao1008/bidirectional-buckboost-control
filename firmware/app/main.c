/**
 * @file main.c
 * @brief Gate 1 firmware entry point for safe host communication bring-up.
 */

#include "clock.h"
#include "host_service.h"
#include "power_stage.h"
#include "timebase.h"
#include "uart.h"

int main(void)
{
    /* Safety-critical initialization comes first. */
    platform_power_stage_init_safe();

    platform_clock_init();
    platform_timebase_init();
    platform_uart_init(115200U);
    host_service_init();

    for (;;) {
        host_service_run();
    }
}
