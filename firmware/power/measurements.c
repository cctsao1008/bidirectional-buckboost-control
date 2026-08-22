/**
 * @file measurements.c
 * @brief Raw ADC to signed engineering-unit measurement conversion.
 */

#include "measurements.h"

#include <stddef.h>

/*
 * Vendor firmware displays a full-scale voltage channel as approximately
 * 68.00 V and a full-scale current span as approximately 22.00 A. These
 * values agree with the V1.2 schematic ratios:
 *
 *   voltage: 3.3 V / (3.3k / 68k) ~= 68 V
 *   current: 3.3 V / (150 * 1 mOhm) = 22 A
 *
 * Use 4096 counts as the nominal 12-bit scaling denominator. Calibration
 * later replaces these nominal constants; control code must not depend on
 * them directly.
 */
#define ADC_COUNTS_NOMINAL          4096.0f
#define PORT_VOLTAGE_FULL_SCALE_V     68.0f
#define PORT_CURRENT_SPAN_A           22.0f
#define ADC_REFERENCE_NOMINAL_V        3.3f
#define CURRENT_ZERO_CODE_NOMINAL   2048.0f

static float convert_channel(const measurement_channel_cal_t *calibration,
                             uint16_t raw_code)
{
    return (((float)raw_code - calibration->zero_code)
            * calibration->scale_per_count)
           + calibration->physical_offset;
}

void measurements_nominal_calibration(measurement_calibration_t *calibration)
{
    if (calibration == NULL) {
        return;
    }

    calibration->vin.scale_per_count = PORT_VOLTAGE_FULL_SCALE_V / ADC_COUNTS_NOMINAL;
    calibration->vin.zero_code = 0.0f;
    calibration->vin.physical_offset = 0.0f;

    calibration->iin.scale_per_count = PORT_CURRENT_SPAN_A / ADC_COUNTS_NOMINAL;
    calibration->iin.zero_code = CURRENT_ZERO_CODE_NOMINAL;
    calibration->iin.physical_offset = 0.0f;

    calibration->vout = calibration->vin;
    calibration->iout = calibration->iin;

    calibration->vadj.scale_per_count = ADC_REFERENCE_NOMINAL_V / ADC_COUNTS_NOMINAL;
    calibration->vadj.zero_code = 0.0f;
    calibration->vadj.physical_offset = 0.0f;
}

void measurements_set_current_zero(measurement_calibration_t *calibration,
                                   float iin_zero_code,
                                   float iout_zero_code)
{
    if (calibration == NULL) {
        return;
    }

    calibration->iin.zero_code = iin_zero_code;
    calibration->iout.zero_code = iout_zero_code;
}

void measurements_convert(const measurement_calibration_t *calibration,
                          const measurement_raw_t *raw,
                          power_measurements_t *output)
{
    if (calibration == NULL || raw == NULL || output == NULL) {
        return;
    }

    output->vin_v = convert_channel(&calibration->vin, raw->vin);
    output->iin_a = convert_channel(&calibration->iin, raw->iin);
    output->vout_v = convert_channel(&calibration->vout, raw->vout);
    output->iout_a = convert_channel(&calibration->iout, raw->iout);
    output->vadj_v = convert_channel(&calibration->vadj, raw->vadj);
}
