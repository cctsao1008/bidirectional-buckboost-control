/**
 * @file clock.h
 * @brief STM32F334 system-clock initialization.
 */

#ifndef BUCKBOOST_CLOCK_H
#define BUCKBOOST_CLOCK_H

/** Configure the STM32F334 system clock to 64 MHz from the internal HSI PLL. */
void platform_clock_init(void);

#endif /* BUCKBOOST_CLOCK_H */
