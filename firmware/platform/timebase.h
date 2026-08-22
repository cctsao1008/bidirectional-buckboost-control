/**
 * @file timebase.h
 * @brief Millisecond system timebase for background services.
 */

#ifndef BUCKBOOST_TIMEBASE_H
#define BUCKBOOST_TIMEBASE_H

#include <stdint.h>

/** Configure SysTick for a 1 kHz millisecond timebase. */
void platform_timebase_init(void);

/** Return milliseconds elapsed since firmware startup. */
uint32_t platform_uptime_ms(void);

#endif /* BUCKBOOST_TIMEBASE_H */
