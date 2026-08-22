/**
 * @file main.c
 * @brief Gate 1 firmware entry point for safe host communication bring-up.
 */

#include "board_io.h"
#include "clock.h"
#include "host_service.h"
#include "power_stage.h"
#include "timebase.h"
#include "uart.h"

int main(void)
{
    /*
     * Safety-critical startup order:
     *   1. Force all gate-driver logic inputs inactive.
     *   2. Establish the system clock.
     *   3. Put all verified non-power-stage GPIOs in deterministic safe states.
     *   4. Start the software timebase.
     *   5. Hand PB6/PB7 from passive GPIO state to USART1.
     *   6. Start the host protocol service.
     */
    platform_power_stage_init_safe();
    platform_clock_init();
    platform_board_io_init_safe();
    platform_timebase_init();
    platform_uart_init(115200U);
    host_service_init();

    for (;;) {
        host_service_run();
    }
}
