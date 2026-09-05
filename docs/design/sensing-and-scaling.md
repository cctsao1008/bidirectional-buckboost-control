# Sensing and Scaling

## Purpose

This document defines the measurement path from ADC codes to calibrated physical quantities. Sign conventions are owned by `control-conventions.md`; estimator design is owned by `current-observability-and-estimation.md`.

## Measurement channels

| MCU pin | Signal | Physical quantity |
| --- | --- | --- |
| PA0 | `ADC_Vin` | Port A voltage |
| PA1 | `ADC_Iin` | Port A current |
| PA2 | `ADC_Vout` | Port B voltage |
| PA3 | `ADC_Iout` | Port B current |
| PA4 | `ADC_VADJ` | local reference input |

The board has no dedicated ADC channel for main-inductor current `iL`.

## Current semantics

```text
Iin  > 0 : current enters the converter from Port A
Iout > 0 : current leaves the converter into Port B
```

Negative current is valid. Direction changes never swap ADC channels or alter sign definitions.

## Nominal voltage scaling

```text
Kv = 3.3 kΩ / 68 kΩ
   ≈ 0.04853

Vadc  ≈ Kv * Vport
Vport ≈ Vadc / Kv
```

A nominal 3.3 V ADC input corresponds to approximately 68 V at the sense input. This is sensing range, not converter operating rating.

## Nominal current scaling

Each terminal-current path uses a 1 mΩ shunt and approximately 150 V/V differential gain:

```text
Ki = 150 * 1 mΩ
   = 0.150 V/A

Vadc = Vbias + Ki * I
Vbias ≈ 1.65 V
```

| Current | Nominal ADC voltage |
| ---: | ---: |
| -5 A | 0.90 V |
| 0 A | 1.65 V |
| +5 A | 2.40 V |

Measured calibration supersedes nominal gain and offset.

## ADC-code conversion

For a 12-bit ADC:

```text
Vadc = adc_code * Vref / 4095
```

Nominal voltage conversion:

```text
Vport = adc_code * Vref / 4095 / Kv
```

Nominal current conversion:

```text
I = (adc_code * Vref / 4095 - Vbias) / Ki
```

Firmware does not assume exact 3.300 V, exact resistor ratios, or exact 1.650 V bias when calibrated values are available.

## Calibration representation

Each channel uses an affine calibrated representation:

```text
physical_value = scale * adc_code + offset
```

Calibration data identifies:

```text
channel
scale
offset
polarity
calibration identity
calibration conditions
```

Controllers and estimators consume calibrated physical quantities only.

## Zero-current calibration

Current-offset calibration is valid only when the system can assert a zero-current condition. Pre-biased or externally energized terminals invalidate an automatic zero-current assumption.

The calibration layer never silently forces the current estimate to zero while energy may be flowing.

## Analog measurement path

```text
physical quantity
      ↓
shunt / divider
      ↓
GS8552 signal conditioning
      ↓
analog RC network
      ↓
ADC sample-and-hold
      ↓
scan / conversion latency
      ↓
DMA
      ↓
calibration / bounded filtering
```

The effective sensing transfer function includes analog settling, RC filtering, ADC loading, conversion timing, and PWM-synchronous disturbance.

## PWM-synchronized acquisition

The acquisition architecture is synchronized to HRTIM.

Each measurement set has defined:

```text
PWM frequency
ADC trigger source
trigger phase
channel conversion order
sample time
conversion latency
DMA completion point
timestamp semantics
control consume point
```

The four ADC1 channels are sequential conversions and are not modeled as simultaneous samples.

## Sampling-phase policy

The sample phase is selected to provide a deterministic relationship to the switching state while avoiding switching-edge disturbance. If the available quiet window changes with duty allocation, the measurement model retains the resulting phase/skew rather than assuming ideal simultaneous values.

## Digital filtering

Any digital filter in the control path has explicit bandwidth and delay. Filtering does not redefine calibration, hide ADC saturation, or compensate for unknown sample timing.

## Estimator boundary

```text
ADC / DMA
   ↓
calibrated Vin / Iin / Vout / Iout
   ↓
bounded filtering
   ↓
state estimator
   ↓
iL_hat + confidence
   ↓
controller
```

## Measurement validity requirements

A measurement set is valid for closed-loop use only when:

- channel mapping is fixed and correct;
- current polarity follows the canonical convention;
- gain/offset calibration is valid;
- signed current is not clipped;
- ADC values are within defined plausibility/saturation bounds;
- sample phase and channel latency are deterministic;
- data timestamp/period association is defined.

Invalid sensing propagates explicit validity state to estimator, protection, and Power Manager.