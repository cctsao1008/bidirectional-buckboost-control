/**
 * @file board_io.h
 * @brief Safe initialization for non-power-stage board GPIOs.
 */

#ifndef BUCKBOOST_BOARD_IO_H
#define BUCKBOOST_BOARD_IO_H

/**
 * Initialize verified board GPIOs to deterministic, non-active states.
 *
 * This function intentionally does not touch:
 *   - PA8..PA11 power-stage gate inputs (owned by power_stage.c)
 *   - PA13/PA14 SWD pins
 *   - GPIOs whose board function has not yet been verified from the V1.2 schematic
 */
void platform_board_io_init_safe(void);

/** Force all three status LEDs off. */
void platform_status_leds_all_off(void);

#endif /* BUCKBOOST_BOARD_IO_H */
