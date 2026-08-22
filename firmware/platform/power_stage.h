/**
 * @file power_stage.h
 * @brief Safe startup control for the converter gate-drive inputs.
 */

#ifndef BUCKBOOST_POWER_STAGE_H
#define BUCKBOOST_POWER_STAGE_H

/**
 * Configure all four gate-driver logic inputs as GPIO outputs held low.
 *
 * This function is intended to be called as the first hardware initialization
 * step after reset, before clocks, communication, ADC, or HRTIM setup.
 */
void platform_power_stage_init_safe(void);

/**
 * Force all four gate-driver logic inputs low while they are GPIO-controlled.
 *
 * Once HRTIM alternate-function control is introduced, this function must be
 * extended to disable/force the HRTIM outputs before relying on it as a fault
 * shutdown path.
 */
void platform_power_stage_force_off(void);

#endif /* BUCKBOOST_POWER_STAGE_H */
