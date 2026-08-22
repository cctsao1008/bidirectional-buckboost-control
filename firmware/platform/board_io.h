/**
 * @file board_io.h
 * @brief Safe initialization for non-power-stage board GPIOs.
 */

#ifndef BUCKBOOST_BOARD_IO_H
#define BUCKBOOST_BOARD_IO_H

#include <stdbool.h>

/**
 * Initialize verified board GPIOs to deterministic, non-active states.
 *
 * This function intentionally does not touch:
 *   - PA8..PA11 power-stage gate inputs (owned by power_stage.c)
 *   - PA13/PA14 SWD pins
 */
void platform_board_io_init_safe(void);

/** Force all three status LEDs off. */
void platform_status_leds_all_off(void);

/** Return true while the active-low KEY1 input is asserted. */
bool platform_key1_pressed(void);

/** Return true while the active-low KEY2 input is asserted. */
bool platform_key2_pressed(void);

#endif /* BUCKBOOST_BOARD_IO_H */
