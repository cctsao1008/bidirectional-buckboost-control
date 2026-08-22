/**
 * @file main.c
 * @brief Gate 1 firmware entry point for host communication bring-up.
 */

#include "clock.h"
#include "host_service.h"
#include "timebase.h"
#include "uart.h"

int main(void)
{
    platform_clock_init();
    platform_timebase_init();
    platform_uart_init(115200U);
    host_service_init();

    for (;;) {
        host_service_run();
    }
}
