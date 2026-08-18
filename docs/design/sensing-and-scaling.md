# Sensing and Scaling

## Scope

This document defines the measurement conventions used by the project for voltage and current sensing. It separates raw ADC behavior, analog conditioning, engineering-unit conversion, and later estimator design.

The sensing path is treated as part of the effective plant rather than as an ideal measurement source.

## Measurement Channels

The initial hardware provides five ADC inputs:

| MCU input | Signal | Purpose |
| --- | --- | --- |
| PA0 | ADC_Vin | Input-port voltage |
| PA1 | ADC_Iin | Input-port current |
| PA2 | ADC_Vout | Output-port voltage |
| PA3 | ADC_Iout | Output-port current |
| PA4 | ADC_VADJ | Potentiometer / reference input |

The two voltage channels use the same general conditioning approach, and the two current channels use the same bidirectional sensing approach.

## Voltage Sensing

The voltage-sensing network uses a differential scaling stage with an approximate ratio:

```text
Kv = 3.3 kΩ / 68 kΩ
   ≈ 0.04853
```

Therefore the ideal conditioned voltage is approximately:

```text
Vadc = Kv × Vport
```

and the corresponding engineering-unit conversion is:

```text
Vport = Vadc / Kv
```

For a 3.3 V ADC full-scale input, the ideal full-scale port voltage is approximately:

```text
3.3 / 0.04853 ≈ 68 V
```

The actual conversion must account for ADC reference accuracy, resistor tolerance, op-amp behavior, and calibration.

## Bidirectional Current Sensing

Each power port uses a 1 mΩ current shunt followed by differential amplification.

Nominal values:

```text
Rshunt = 1 mΩ
Amplifier gain ≈ 150
```

This gives an approximate current sensitivity:

```text
Ki = 150 × 1 mΩ
   = 0.15 V/A
```

Because converter current can flow in either direction, the measurement is centered around a nominal 1.65 V offset:

```text
Vadc = 1.65 + 0.15 × I
```

and therefore:

```text
I = (Vadc - 1.65) / 0.15
```

With the nominal scaling:

| Current | Ideal ADC voltage |
| ---: | ---: |
| -5 A | 0.90 V |
| 0 A | 1.65 V |
| +5 A | 2.40 V |

This leaves headroom on both sides of the rated ±5 A operating range.

## Current Sign Convention

A single sign convention must be used consistently across firmware, models, and tests.

The initial project convention should define positive current relative to the selected forward power-flow direction. Reverse operation should change the power-flow state rather than silently reinterpret ADC polarity.

The exact mapping between physical shunt polarity and software-positive current must be verified experimentally before closed-loop current control is enabled.

## ADC Conversion

For an `N`-bit ADC with reference voltage `Vref`:

```text
Vadc = adc_code × Vref / (2^N - 1)
```

For the STM32F334 12-bit ADC and nominal 3.3 V reference:

```text
Vadc = adc_code × 3.3 / 4095
```

The voltage channel can then be converted as:

```text
Vport = adc_code × Vref / 4095 / Kv
```

The current channel can be converted as:

```text
I = (adc_code × Vref / 4095 - Vbias) / Ki
```

These equations are nominal. Production-quality conversion should use calibrated gain and offset values rather than assuming exact resistor ratios or exact 1.65 V bias.

## Calibration Model

The preferred calibration representation is affine:

```text
physical_value = scale × adc_code + offset
```

For each channel, store at least:

```text
zero offset
scale / gain
calibration temperature
calibration date or dataset ID
```

This keeps calibration separate from the controller and allows measured correction values to replace nominal design values without changing control code.

## Analog Filtering

The conditioning circuits include RC filtering at the op-amp outputs. These filters reduce switching noise but introduce amplitude and phase dynamics.

For control purposes, the measurement path should therefore be modeled as:

```text
physical variable
      ↓
shunt / divider
      ↓
op-amp conditioning
      ↓
analog RC filter
      ↓
ADC sample-and-hold
      ↓
digital scaling / filtering
```

The effective bandwidth should be measured or calculated from the actual component values and validated against injected or naturally occurring transients.

## Op-Amp Limits

The signal-conditioning path uses a GS8552 zero-drift amplifier family device.

Relevant non-idealities for control work include:

- finite gain-bandwidth;
- finite slew rate;
- input offset and offset drift;
- output settling;
- common-mode behavior;
- resistor-network mismatch;
- capacitive loading from the RC output network and ADC input.

The op-amp datasheet bandwidth is not assumed to equal the usable sensing bandwidth of the complete circuit.

## Sampling Strategy

ADC timing should be synchronized with PWM where practical.

Sampling phase matters because switch-node transitions, diode recovery, gate-drive edges, and inductor ripple can introduce deterministic measurement noise.

The eventual implementation should document:

```text
PWM frequency
ADC trigger source
sample phase within PWM period
ADC acquisition time
conversion latency
control-loop update rate
```

These timing parameters are part of the control design and must be reproducible.

## Estimation Boundary

Raw sensing and state estimation are intentionally separate.

```text
ADC acquisition
    ↓
calibrated measurements
    ↓
optional digital filtering
    ↓
state estimator
    ↓
control law
```

A Kalman filter, Luenberger observer, or other estimator should consume calibrated measurements; it should not be used to hide unknown sensor scaling or unverified offsets.

## Measurement Validation

Before using a channel for closed-loop control, validate at minimum:

- zero offset;
- static gain;
- polarity;
- linearity across the intended range;
- noise with PWM off;
- noise with PWM active;
- transient response / bandwidth;
- channel-to-channel consistency where applicable.

## Source of Truth

Nominal design values will eventually be stored in a machine-readable parameter file. Measured calibration values and experiment-specific conditions should be stored separately so that nominal hardware parameters are not silently overwritten by one test setup.
