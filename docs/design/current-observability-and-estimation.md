# Inductor-Current Observability and Estimation

## Purpose

The CBB024D V1.2 board measures `Vin`, `Iin`, `Vout`, and `Iout`, but does not directly sample the main inductor current `iL`.

This document defines the project strategy for reconstructing `iL` accurately enough for cascaded current control and later LQI, Super-Twisting SMC, Deadbeat, and MPC without adding a current sensor.

The hardware constraint is fixed:

> **The final architecture uses only the existing board measurements and converter state. No additional inductor-current sensor or ADC channel is added.**

## Available information

Measurements:

```text
Vin
Iin
Vout
Iout
```

Known control/state information:

```text
d1
d2
PWM period
switching / operating state
```

Required reconstructed state:

```text
iL_hat
```

All signs and duty definitions follow `control-conventions.md`.

## Why terminal current is not inductor current

The two 1 mΩ shunts measure port current. The port capacitors can source or absorb current, so neither terminal-current channel is an unconditional instantaneous measurement of `iL`.

Using the project conventions, an ideal averaged three-state model is:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

This is the canonical estimator backbone. Loss terms such as inductor DCR, MOSFET conduction loss, dead-time voltage error, and capacitor ESR are added only when measured data shows that they materially improve prediction.

## Algebraic information from the port equations

The capacitor equations imply:

```text
iL_from_A = (Iin - Cin dVin/dt) / d1

iL_from_B = (Iout + Cout dVout/dt) / (1 - d2)
```

These are useful analysis relations and may be used as bounded pseudo-measurements or consistency checks, but they are not suitable as unconditional real-time formulas because:

- differentiation amplifies voltage noise;
- `Cin` and `Cout` are uncertain and voltage dependent;
- `d1` or `1-d2` can become small;
- ADC channels have nonzero sequence latency;
- switching ripple depends on sample phase;
- DCM and zero-current crossings violate simple CCM assumptions.

The implementation must therefore avoid blind division by small duty terms and must track data quality.

## Preferred estimator structure

The preferred first estimator is a model predictor with measurement correction.

State vector:

```text
x = [ Vin, iL, Vout ]^T
```

Measured model inputs:

```text
u = [ Iin, Iout, d1, d2 ]
```

Direct measured states available for residual correction:

```text
y = [ Vin, Vout ]^T
```

Continuous predictor:

```text
dVin/dt  = (Iin - d1 iL) / Cin

diL/dt   = (d1 Vin - (1 - d2) Vout - Rl iL) / L

dVout/dt = ((1 - d2) iL - Iout) / Cout
```

`Iin` and `Iout` are therefore measured inputs to the model, not direct `iL` samples. Measured `Vin` / `Vout` residuals correct the predicted states. Duty-conditioned algebraic current estimates may later provide additional correction information when their numerical conditioning and bandwidth are acceptable.

## Discrete baseline predictor

At sample period `Ts`, a first forward-Euler implementation is:

```text
Vin_hat[k+1] = Vin_hat[k]
              + Ts/Cin * (Iin[k] - d1[k] iL_hat[k])

iL_hat[k+1] = iL_hat[k]
              + Ts/L * (d1[k] Vin_hat[k]
                        - (1 - d2[k]) Vout_hat[k]
                        - Rl iL_hat[k])

Vout_hat[k+1] = Vout_hat[k]
               + Ts/Cout * ((1 - d2[k]) iL_hat[k] - Iout[k])
```

The integration method may later be improved if the error is material relative to the control bandwidth.

## Candidate correction methods

Use the simplest method that meets error and phase requirements:

```text
complementary predictor-corrector
Luenberger / linear time-varying observer
Kalman filter
extended Kalman filter
```

Do not start with EKF merely because it is available. Estimator complexity must be justified by measured improvement.

## Observability and confidence

The hidden inductor state influences both capacitor-voltage dynamics:

```text
dVin/dt  contains -d1 iL
dVout/dt contains +(1 - d2) iL
```

Observability weakens when the corresponding coupling term approaches zero:

```text
d1 -> 0
1 - d2 -> 0
```

The estimator interface should therefore expose at least:

```text
iL_hat
confidence
valid
flags
```

Low-confidence conditions include startup before offset calibration, ADC saturation, poor duty conditioning, DCM/zero crossing outside the validated model, timing uncertainty, or measurement plausibility failure.

## Nominal plant parameters

Initial schematic-derived values:

```text
L    = 22 uH nominal, ±20%
Cin  ≈ 460 uF nominal including 2 x 220 uF + 2 x 10 uF
Cout ≈ 460 uF nominal including 2 x 220 uF + 2 x 10 uF
fsw  = 200 kHz
Ts   = 5 us for one update per switching cycle
```

These are starting parameters, not identified truth. Ceramic DC-bias derating, electrolytic tolerance, ESR, and effective source/load impedance can materially alter the useful model.

## Vendor evidence retained because it matters

Only vendor evidence relevant to the new estimator architecture is carried forward:

- ADC1 samples `Vin`, `Iin`, `Vout`, and `Iout` with DMA;
- the ADC trigger is tied to HRTIM timing;
- vendor code places the sample away from switching edges;
- vendor firmware performs zero-current offset averaging;
- forward reference code clips negative current while reverse reference code changes interpretation.

The project keeps the useful timing and offset-calibration concepts while rejecting direction-dependent channel remapping and negative-current clipping.

## Measurement prerequisites

Before an `iL_hat`-dependent controller is energized, establish:

1. current-channel zero offsets;
2. current polarity under the fixed Port A / Port B convention;
3. exact ADC conversion order and latency;
4. PWM phase of every ADC sample;
5. noise with PWM inactive and active;
6. whether per-cycle samples are useful directly or require filtering/decimation;
7. effective `L`, `Cin`, and `Cout` only to the accuracy required by the estimator;
8. estimator delay, residuals, and failure modes.

These are estimator-enabling measurements, not a repetition of vendor power-stage qualification.

## Development-only validation instrumentation

An external current probe may be used during development to score `iL_hat` error, phase delay, and transient behavior. Such a probe is validation instrumentation only and is **not** part of the final control architecture or sensing requirements.

## Controller gating

| Controller | Estimator requirement |
| --- | --- |
| Voltage-loop PI | Does not require `iL_hat` |
| Cascaded V/I PI | Bounded delay/error over intended bandwidth |
| LQI | Observer quality must support chosen state model |
| ST-SMC | Bounded signed-current error and noise |
| Deadbeat | High per-cycle accuracy and low latency |
| MPC | State quality characterized over the optimization envelope |

Deadbeat and MPC therefore follow, rather than precede, estimator validation.

## Efficient development sequence

```text
PWM-synchronized signed ADC acquisition
        ↓
offset / gain calibration
        ↓
timestamp and PWM-phase verification
        ↓
log Vin / Iin / Vout / Iout / d1 / d2
        ↓
offline estimator development
        ↓
quantify error / delay / confidence behavior
        ↓
port estimator to firmware
        ↓
cascaded PI or LQI
```

## Estimator gate

The estimator is accepted only when its intended operating envelope demonstrates:

```text
bounded estimation error
acceptable phase delay for target current-loop bandwidth
correct sign in both power directions
stable behavior through Buck/Mixed/Boost transitions
explicit low-confidence detection outside the validated envelope
```

Numeric thresholds are derived from controller and protection requirements after the acquisition path is measured; they are not arbitrary constants in this document.

## Do not

- add a final-architecture `iL` sensor;
- treat `Iin` or `Iout` as instantaneous `iL`;
- divide blindly by small duty terms;
- differentiate voltage without bandwidth control;
- clip negative current;
- hide unknown sensor scaling inside an observer;
- enable estimator-dependent advanced control before acquisition timing and calibration are credible;
- spend time re-proving vendor-validated basic converter operation.
