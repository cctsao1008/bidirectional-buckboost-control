/**
 * @file measurements.h
 * @brief Raw ADC to signed engineering-unit measurement conversion.
 */

#ifndef BUCKBOOST_MEASUREMENTS_H
#define BUCKBOOST_MEASUREMENTS_H

#include <stdint.h>

typedef struct {
    float scale_per_count;
    float zero_code;
    float physical_offset;
} measurement_channel_cal_t;

typedef struct {
    measurement_channel_cal_t vin;
    measurement_channel_cal_t iin;
    measurement_channel_cal_t vout;
    measurement_channel_cal_t iout;
    measurement_channel_cal_t vadj;
} measurement_calibration_t;

typedef struct {
    uint16_t vin;
    uint16_t iin;
    uint16_t vout;
    uint16_t iout;
    uint16_t vadj;
} measurement_raw_t;

typedef struct {
    float vin_v;
    float iin_a;
    float vout_v;
    float iout_a;
    float vadj_v;
} power_measurements_t;

/** Load schematic/vendor-derived nominal conversion values. */
void measurements_nominal_calibration(measurement_calibration_t *calibration);

/** Update measured zero-current ADC codes without changing current gain. */
void measurements_set_current_zero(measurement_calibration_t *calibration,
                                   float iin_zero_code,
                                   float iout_zero_code);

/** Convert one coherent raw sample set to engineering units. */
void measurements_convert(const measurement_calibration_t *calibration,
                          const measurement_raw_t *raw,
                          power_measurements_t *output);

#endif /* BUCKBOOST_MEASUREMENTS_H */
