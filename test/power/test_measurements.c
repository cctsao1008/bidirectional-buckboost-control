/**
 * @file test_measurements.c
 * @brief Host-side tests for nominal signed measurement conversion.
 */

#include <assert.h>
#include <stdio.h>

#include "../../firmware/power/measurements.h"

static int nearf(float a, float b, float tolerance)
{
    const float delta = (a > b) ? (a - b) : (b - a);
    return delta <= tolerance;
}

static void test_nominal_zero_and_voltage(void)
{
    measurement_calibration_t calibration;
    measurement_raw_t raw = {0};
    power_measurements_t measurement = {0};

    measurements_nominal_calibration(&calibration);

    raw.vin = 2048U;
    raw.iin = 2048U;
    raw.vout = 1024U;
    raw.iout = 2048U;
    raw.vadj = 2048U;

    measurements_convert(&calibration, &raw, &measurement);

    assert(nearf(measurement.vin_v, 34.0f, 0.001f));
    assert(nearf(measurement.vout_v, 17.0f, 0.001f));
    assert(nearf(measurement.iin_a, 0.0f, 0.001f));
    assert(nearf(measurement.iout_a, 0.0f, 0.001f));
    assert(nearf(measurement.vadj_v, 1.65f, 0.001f));
}

static void test_signed_current_is_preserved(void)
{
    measurement_calibration_t calibration;
    measurement_raw_t raw = {0};
    power_measurements_t measurement = {0};

    measurements_nominal_calibration(&calibration);

    raw.iin = 2979U;  /* approximately +5 A */
    raw.iout = 1117U; /* approximately -5 A */

    measurements_convert(&calibration, &raw, &measurement);

    assert(nearf(measurement.iin_a, 5.000f, 0.01f));
    assert(nearf(measurement.iout_a, -5.000f, 0.01f));
}

static void test_measured_current_zero(void)
{
    measurement_calibration_t calibration;
    measurement_raw_t raw = {0};
    power_measurements_t measurement = {0};

    measurements_nominal_calibration(&calibration);
    measurements_set_current_zero(&calibration, 2056.0f, 2039.0f);

    raw.iin = 2056U;
    raw.iout = 2039U;

    measurements_convert(&calibration, &raw, &measurement);

    assert(nearf(measurement.iin_a, 0.0f, 0.001f));
    assert(nearf(measurement.iout_a, 0.0f, 0.001f));
}

int main(void)
{
    test_nominal_zero_and_voltage();
    test_signed_current_is_preserved();
    test_measured_current_zero();

    puts("measurement tests: PASS");
    return 0;
}
