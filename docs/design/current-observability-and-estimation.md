# Inductor-Current Observability and Estimation

## Purpose

The CBB024D V1.2 board measures `Vin`, `Iin`, `Vout`, and `Iout` but does not directly sample the main-inductor current `iL`.

The control architecture reconstructs `iL` from the existing measurements, converter model, realized duty commands, and switching timing. No permanent inductor-current sensor or additional ADC channel is part of the architecture.

## Available information

Measurements:

```text
Vin
Iin
Vout
Iout
```

Known actuation/timing:

```text
d1
d2
e1 = d1
e2 = 1 - d2
PWM period / sample timing
```

Estimator output:

```text
iL_hat
confidence
valid
flags
```

All signs follow `control-conventions.md`.

## Canonical estimator model

The ideal averaged three-state model is:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

State and measured-input definitions are:

```text
x = [ Vin, iL, Vout ]^T
u = [ Iin, Iout, d1, d2 ]
y = [ Vin, Vout ]^T
```

The fast inductor predictor uses the actuation actually realized by modulation:

```text
vL_realized = Vin e1 - Vout e2

diL_hat/dt = (vL_realized - Rl iL_hat) / L
```

Using `vL_realized` keeps the estimator consistent with duty saturation and allocator constraints.

## Predictor-correction structure

```text
realized e1/e2 + Vin/Vout
          ↓
physics predictor
          ↓
iL_pred
          ↓
conditioned measurement correction
          ↓
iL_hat + confidence
```

The predictor is the fast path. Terminal measurements provide lower-bandwidth correction and consistency information.

## Why terminal current is not `iL`

The 1 mΩ shunts measure port current. Port capacitors can source or absorb current, so neither terminal-current channel is an unconditional instantaneous measurement of the main-inductor current.

The port equations imply:

```text
iL_from_A = (Iin - Cin dVin/dt) / d1

iL_from_B = (Iout + Cout dVout/dt) / (1 - d2)
```

These relations are conditioned pseudo-measurements, not unconditional per-cycle formulas. Their validity is reduced by:

- voltage differentiation noise;
- uncertain effective `Cin` / `Cout`;
- small `d1` or `1-d2` denominators;
- ADC channel sequence skew;
- switching-ripple sample phase;
- DCM and zero-current behavior outside the CCM model.

Pseudo-measurements are ignored or down-weighted when their conditioning is poor.

## Discrete predictor

For sample period `Ts`:

```text
Vin_hat[k+1] = Vin_hat[k]
              + Ts/Cin * (Iin[k] - d1[k] iL_hat[k])

iL_hat[k+1] = iL_hat[k]
              + Ts/L * (vL_realized[k] - Rl iL_hat[k])

Vout_hat[k+1] = Vout_hat[k]
               + Ts/Cout * ((1 - d2[k]) iL_hat[k] - Iout[k])
```

The estimator consumes the same timestamped measurement set and realized duty state used by the control cycle.

## Observability and confidence

The hidden current state couples into the capacitor dynamics through:

```text
dVin/dt  contains -d1 iL
dVout/dt contains +(1 - d2) iL
```

Correction observability weakens as:

```text
d1 -> 0
1 - d2 -> 0
```

`confidence`, `valid`, and `flags` therefore reflect at least:

- ADC validity and saturation;
- current-offset/calibration validity;
- measurement timestamp validity;
- duty conditioning;
- model-envelope validity;
- residual plausibility;
- DCM/zero-crossing conditions outside the accepted CCM envelope.

A controller that requires `iL_hat` does not use the estimate while `valid == false`.

## Nominal plant parameters

```text
L    = 22 uH nominal, ±20%
Cin  ≈ 460 uF nominal per Port A network
Cout ≈ 460 uF nominal per Port B network
fsw  = 200 kHz
Ts   = 5 us for one update per switching cycle
```

Nominal values are model inputs, not calibrated truth. Effective capacitance, ESR, inductor DCR, inductance, source impedance, and load impedance remain explicit parameter uncertainties.

## Measurement requirements

The estimator consumes calibrated signed measurements with defined:

```text
channel polarity
scale / offset
PWM trigger phase
ADC conversion order
conversion latency
DMA handoff point
measurement timestamp
```

Unknown timing or polarity invalidates estimator confidence.

## Validation instrumentation

An external Hall/current probe may be connected at the board's inductor-current measurement access point as an independent validation reference. It is not part of the runtime sensing architecture.

## Acceptance conditions

Within a stated operating envelope, `iL_hat` is usable only when it demonstrates:

```text
correct sign in both power directions
bounded estimation error
bounded phase delay
stable behavior across the defined CCM duty envelope
explicit invalid/low-confidence behavior outside that envelope
```

Numeric limits are controller-specific configuration and validation data; they are not embedded as undocumented constants in the estimator architecture.