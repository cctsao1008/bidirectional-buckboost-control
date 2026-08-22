# Sensing and Scaling

## Purpose

This document defines the measurement path from ADC codes to calibrated physical quantities. It owns acquisition/scaling semantics, not estimator design and not current/power sign conventions.

Canonical signs are defined in `control-conventions.md` and must not be redefined locally.

## Measurement Channels

| MCU pin | Signal | Physical quantity |
| --- | --- | --- |
| PA0 | `ADC_Vin` | Port A voltage |
| PA1 | `ADC_Iin` | Port A current |
| PA2 | `ADC_Vout` | Port B voltage |
| PA3 | `ADC_Iout` | Port B current |
| PA4 | `ADC_VADJ` | local potentiometer/reference input |

The board provides no dedicated ADC channel for main-inductor current `iL`.

## Canonical Current Meaning

Per `control-conventions.md`:

```text
Iin  > 0 : current enters the converter from Port A
Iout > 0 : current leaves the converter into Port B
```

Reverse power flow therefore produces valid negative current values. The common sensing layer must never clip them to zero.

The physical polarity of each shunt/amplifier channel must be verified once during measurement bring-up, then encoded in calibration. Direction changes never swap ADC channels.

## Nominal Voltage Scaling

The voltage-conditioning network has an approximate ratio:

```text
Kv = 3.3 kΩ / 68 kΩ
   ≈ 0.04853
```

Nominal conversion:

```text
Vadc  ≈ Kv * Vport
Vport ≈ Vadc / Kv
```

A nominal 3.3 V ADC full scale corresponds to about 68 V at the sense input. This is a signal-conditioning full-scale estimate, not a converter operating-voltage rating.

## Nominal Bidirectional Current Scaling

Each terminal-current path uses a 1 mΩ shunt and approximately 150 V/V differential gain:

```text
Ki = 150 * 1 mΩ
   = 0.150 V/A
```

The current channels are centered around a nominal 1.65 V bias:

```text
Vadc = 1.65 + Ki * I
I    = (Vadc - 1.65) / Ki
```

Nominal examples:

| Current | ADC voltage |
| ---: | ---: |
| -5 A | 0.90 V |
| 0 A | 1.65 V |
| +5 A | 2.40 V |

Measured offset and gain supersede the nominal values in firmware.

## ADC-Code Conversion

For the 12-bit STM32F334 ADC:

```text
Vadc = adc_code * Vref / 4095
```

Nominal voltage channel:

```text
Vport = adc_code * Vref / 4095 / Kv
```

Nominal current channel:

```text
I = (adc_code * Vref / 4095 - Vbias) / Ki
```

The implementation should not assume exact 3.300 V reference, exact resistor ratios, or exact 1.650 V current bias when calibrated values are available.

## Calibration Model

The preferred per-channel representation is affine:

```text
physical_value = scale * adc_code + offset
```

Calibration records should contain, as applicable:

```text
channel identity
scale
offset
polarity
calibration dataset/revision
calibration conditions
optional temperature metadata
```

Calibration is a sensing concern. Controllers and estimators consume already calibrated physical quantities.

## Zero-Current Calibration

Both current channels require a measured zero-current offset. A startup/waiting-state average similar to the vendor reference approach is useful, but the independent implementation must explicitly define when the converter is guaranteed to be at zero current and whether pre-biased/reverse-power conditions invalidate that assumption.

Zero-current calibration must never silently run when energy may already be flowing.

## Analog Measurement Path

The effective measurement plant is:

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
scan/conversion latency
      ↓
DMA
      ↓
calibration / optional digital filtering
```

The op-amp datasheet bandwidth alone is not the sensing bandwidth. Common-mode behavior, settling, resistor mismatch, ADC loading, filter phase, and PWM-synchronous disturbance can matter.

## PWM-Synchronized Acquisition

The final acquisition architecture is synchronized with HRTIM rather than free-running background sampling.

The implementation must document:

```text
PWM frequency
ADC trigger source
trigger phase within the switching period
channel conversion order
sample time
conversion latency
DMA completion point
measurement-set timestamp semantics
control-loop consume point
```

A measurement set must have a well-defined relationship to the `d1` / `d2` command and switching period that produced it.

## Sampling-Phase Policy

Sampling phase is selected to support the new estimator/controller, not to reproduce a vendor waveform. The useful point should minimize deterministic switching contamination while preserving known ripple phase and bounded control latency.

If Buck/Mixed/Boost modulation changes the quiet sampling window, the acquisition layer must either schedule the trigger accordingly or document the resulting measurement error.

## Digital Filtering

Filtering is added only when needed. Every digital filter used in the control path must publish its delay/phase effect. Filtering must not be used to hide bad sample timing, unknown gain, or ADC saturation.

## Estimation Boundary

```text
ADC/DMA
   ↓
calibrated Vin / Iin / Vout / Iout
   ↓
optional bounded filtering
   ↓
state estimator
   ↓
iL_hat / estimator confidence
   ↓
controller
```

`current-observability-and-estimation.md` owns the `iL_hat` architecture.

## Measurement Validation Required by New Firmware

Before a measurement is used for closed-loop control, validate the implementation delta:

- correct channel mapping;
- zero offset;
- static gain/polarity;
- no clipping of valid signed current;
- ADC saturation/plausibility behavior;
- deterministic sample phase and channel latency;
- noise/settling only to the extent required by controller bandwidth;
- reproducibility across reset/startup.

This is not a requirement to repeat the vendor’s complete hardware characterization.

## Source-of-Truth Rule

Nominal circuit constants belong in the hardware/parameter source. Measured calibration belongs in calibration data. Experiment-specific corrections must not overwrite nominal hardware facts without provenance.
