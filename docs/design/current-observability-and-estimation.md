# Inductor-Current Observability and Estimation

## Purpose

The CBB024D V1.2 board measures `Vin`, `Iin`, `Vout`, and `Iout`, but it does not directly sample the main inductor current `iL`.

This document defines how the project will determine whether `iL` can be reconstructed accurately enough for cascaded current control, LQI, sliding-mode control, deadbeat control, and predictive control without adding a current sensor.

The objective is not to reproduce vendor control. The vendor examples already demonstrate that the power stage, ADC channels, HRTIM, and classical voltage/current loops operate. The new engineering problem is the estimator/control architecture enabled by the existing measurements.

## Fixed hardware constraint

No additional current sensor is added.

Available measurements:

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
switching state / operating region
```

Required reconstructed state:

```text
iL_hat
```

## Why Iin and Iout are not iL

The two 1 mOhm shunts are port-current sensors. They do not directly measure the current through L1.

Because the power stage includes input and output capacitors, port currents and inductor current differ during transients.

For the sign conventions defined in `control-conventions.md`, an ideal averaged model is:

```text
Cin * dVin/dt   = Iin - d1 * iL
L   * diL/dt    = d1 * Vin - (1 - d2) * Vout
Cout * dVout/dt = (1 - d2) * iL - Iout
```

The practical model will later add inductor DCR, MOSFET conduction loss, dead-time effects, capacitor ESR, and other terms only when measurement evidence shows that they materially improve estimation.

## Algebraic current estimates

The capacitor equations provide two possible algebraic estimates:

```text
iL_from_input  = (Iin - Cin * dVin/dt) / d1

iL_from_output = (Iout + Cout * dVout/dt) / (1 - d2)
```

These relations are useful for analysis and observer correction, but they are not suitable as unconditional real-time formulas because:

- voltage differentiation amplifies noise;
- capacitor values have tolerance and voltage dependence;
- `d1` can become small;
- `1 - d2` can become small;
- ADC channels are sampled sequentially rather than simultaneously;
- switching ripple and sample phase introduce deterministic error;
- discontinuous-conduction and zero-current regions violate simple CCM assumptions.

Therefore the production estimator should not be implemented as a single direct division formula.

## Preferred estimator structure

The initial preferred structure is a model predictor with measurement correction.

State vector:

```text
x = [ Vin, iL, Vout ]^T
```

Measured disturbances / inputs:

```text
u = [ Iin, Iout, d1, d2 ]
```

Measured states available for correction:

```text
y = [ Vin, Vout ]^T
```

The continuous averaged predictor is:

```text
dVin/dt  = (Iin - d1 * iL) / Cin

diL/dt   = (d1 * Vin - (1 - d2) * Vout - Rl * iL) / L

dVout/dt = ((1 - d2) * iL - Iout) / Cout
```

where `Rl` is initially zero or a nominal inductor/path resistance and is introduced only after identification.

This formulation has an important property: the measured port currents are used as measured model inputs rather than being incorrectly treated as direct measurements of `iL`.

## Discrete predictor

For an initial forward-Euler implementation with control period `Ts`:

```text
Vin_hat[k+1] = Vin_hat[k]
             + Ts/Cin * (Iin[k] - d1[k] * iL_hat[k])

iL_hat[k+1] = iL_hat[k]
             + Ts/L * (d1[k] * Vin_hat[k]
                       - (1 - d2[k]) * Vout_hat[k]
                       - Rl * iL_hat[k])

Vout_hat[k+1] = Vout_hat[k]
              + Ts/Cout * ((1 - d2[k]) * iL_hat[k] - Iout[k])
```

Measured `Vin` and `Vout` residuals are then used to correct the predicted state.

The final observer may be implemented as one of:

```text
Luenberger / linear time-varying observer
complementary predictor-corrector
Kalman filter
extended Kalman filter
```

The simplest observer that meets bandwidth and error requirements should be preferred.

## Why this is potentially observable

The inductor state influences both measured capacitor-voltage dynamics:

```text
dVin/dt  depends on -d1 * iL
dVout/dt depends on +(1 - d2) * iL
```

Therefore the unmeasured inductor current leaves a signature in two measured voltage states while both terminal currents are also measured.

Observability degrades near duty configurations where the corresponding coupling coefficient approaches zero:

```text
d1 -> 0
1 - d2 -> 0
```

The estimator must therefore track an observability/confidence metric rather than assuming constant information quality across all operating points.

## Nominal plant parameters

Initial schematic-derived values:

```text
L    = 22 uH nominal, ±20%
Cin  = 2 * 220 uF + 2 * 10 uF = 460 uF nominal
Cout = 2 * 220 uF + 2 * 10 uF = 460 uF nominal
fsw  = 200 kHz
Ts   = 5 us at one update per switching cycle
```

The 10 uF ceramic capacitors may have significant DC-bias derating, and electrolytic capacitance is not exact. Estimator tuning must not assume that 460 uF is an accurate identified value.

## Vendor implementation evidence that matters

Only vendor information relevant to the new estimator architecture is retained:

- ADC1 samples `Vin`, `Iin`, `Vout`, and `Iout` with DMA.
- HRTIM Compare3 is used as the ADC trigger.
- The vendor keeps Compare3 near half of the active Buck compare value to avoid switching edges.
- The forward example calibrates zero-current offset during the waiting state by averaging 256 current samples.
- The forward example clips negative current values to zero.
- The reverse example changes channel interpretation and reverses the current-offset subtraction.

The project keeps the useful zero-offset calibration concept but rejects direction-specific channel remapping and negative-current clipping.

## Measurement requirements before enabling an iL-based controller

The following are estimator-specific requirements, not a re-validation of the vendor power stage:

1. Determine actual zero-current ADC offsets for both shunts.
2. Verify signed current polarity using the fixed Port A / Port B convention.
3. Record exact ADC conversion order and conversion latency for all four channels.
4. Record the PWM phase at which each ADC conversion occurs.
5. Determine effective measurement noise with PWM inactive and active.
6. Determine whether raw per-cycle current samples contain useful information or require decimation/filtering.
7. Identify effective `L`, `Cin`, and `Cout` only to the accuracy needed by the observer.
8. Quantify estimator phase delay and error against an independent bench reference during development if available; that reference is not part of the final architecture.

## Estimator confidence

The estimator should output more than `iL_hat`.

Recommended interface:

```text
iL_hat
confidence
valid
saturation / poor-observability flags
```

A controller that requires high-quality inductor current must be able to reject or degrade gracefully when estimator confidence is low.

Examples of low-confidence conditions:

```text
startup before current-offset calibration
ADC saturation
d1 too close to zero
1 - d2 too close to zero
large sample-timing uncertainty
DCM / zero-current crossing not yet modeled
measurement plausibility failure
```

## Controller gating

Controller enable policy:

| Controller | Required estimator quality |
| --- | --- |
| Voltage-loop PI | Does not require `iL_hat` |
| Cascaded V/I PI | Moderate bandwidth and bounded phase error |
| LQI + observer | Natural fit; estimator is part of controller design |
| ST-SMC | Requires bounded delay/noise and signed current |
| Deadbeat current control | Requires high per-cycle accuracy and low delay |
| Reduced / explicit MPC | Depends on formulation; state quality must be characterized |

Deadbeat is therefore not the first estimator-dependent milestone.

## Efficient development sequence

The project should not spend time re-running vendor demonstrations. The useful sequence is:

```text
Signed ADC acquisition
        ↓
Current-offset calibration
        ↓
Timestamp / PWM-phase correctness
        ↓
Log Vin, Iin, Vout, Iout, d1, d2
        ↓
Offline estimator development in Python
        ↓
Quantify iL_hat error / phase delay
        ↓
Port estimator to firmware
        ↓
Cascaded PI or LQI
```

This allows estimator mathematics and controller design to be changed offline without repeatedly reflashing the converter.

## Pass/fail gate

The estimator gate is passed only when a preregistered operating envelope shows:

```text
bounded current-estimation error
acceptable phase delay for the target current-loop bandwidth
correct current sign in both power directions
stable behavior through Buck/Mix/Boost transitions
explicit low-confidence detection outside the validated envelope
```

No arbitrary numeric threshold is frozen in this document. Thresholds should be set from controller bandwidth and protection requirements after the acquisition path is measured.

## What not to do

Do not:

- add an external current sensor to the final architecture;
- treat `Iin` or `Iout` as instantaneous `iL`;
- divide blindly by very small duty terms;
- differentiate noisy voltages without bandwidth control;
- clip negative current because the current application happens to be forward;
- enable Deadbeat solely because the nominal averaged equation is simple;
- spend time re-proving vendor-validated Buck/Boost/Mix operation.
