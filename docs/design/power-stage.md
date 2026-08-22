# Power Stage

## Purpose

This document defines the project-owned control model of the CBB024D V1.2 four-switch power stage. `hardware-specification.md` owns detailed board facts; this file focuses on the physical topology and equations required by control, estimation, and modulation.

The board is already vendor-proven to operate. Model validation is performed only when required by a new estimator, controller, modulation strategy, or protection function.

## Physical Topology

The converter is a four-switch non-isolated synchronous bidirectional buck-boost stage with two half bridges connected through one inductor:

```text
Port A / left                         Port B / right

      Q1 high                              Q2 high
         |                                    |
         +----------- L1 = 22 uH -------------+
         |                                    |
      Q4 low                               Q3 low
         |                                    |
        GND----------------------------------GND
```

**Canonical V1.2 half-bridge mapping:**

| Bridge | High-side | Low-side | PWM signals |
| --- | --- | --- | --- |
| Left / Port A | Q1 | Q4 | `PWM1H`, `PWM1L` |
| Right / Port B | Q2 | Q3 | `PWM2H`, `PWM2L` |

This correct mapping must be used everywhere. Older conceptual vendor diagrams that imply Q1/Q2 and Q4/Q3 pairings do not represent the physical V1.2 half bridges.

## Nominal Parameters

| Parameter | Value |
| --- | --- |
| Input range | 12–48 VDC |
| Output range | 5–48 VDC |
| Rated operating point | 24 V / 5 A |
| Suggested maximum power | 200 W |
| Switching frequency | 200 kHz |
| Main inductor | 22 µH nominal, ±20 % |
| Inductor DCR | about 20.5 mΩ typ reference value |
| Bulk capacitance | 2 × 220 µF per port, plus local ceramics |
| Terminal-current shunts | 1 mΩ |
| MOSFET | BSC070N10NS3G |
| Gate driver | Si8233BD-D-IS |

Nominal values are starting parameters, not identified truth.

## Canonical Averaged Model

All signs and duty variables follow `control-conventions.md`:

```text
d1 = average on-time fraction of Q1, left high-side
d2 = average on-time fraction of Q3, right low-side
iL > 0 from Port A to Port B
```

Ideal averaged equations:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

The central actuation equation is:

```text
vL = d1 Vin - (1 - d2) Vout
```

At ideal steady state:

```text
Vout / Vin = d1 / (1 - d2)
```

This equation is the common control-allocation backbone for Buck, Mixed, Boost, and reverse power flow.

## Forward Buck-Like Operation

When Port B voltage is sufficiently below Port A voltage, an efficient allocation typically modulates the left bridge while the right bridge remains near a pass-through state compatible with synchronous conduction and bootstrap constraints.

Ideal relation:

```text
Vout / Vin ≈ d1
```

The exact gate state is owned by the modulation layer, not by this plant model.

## Forward Boost-Like Operation

When Port B voltage is sufficiently above Port A voltage, an efficient allocation typically modulates the right bridge while the left bridge remains near the corresponding pass-through state.

Ideal relation under the project duty convention:

```text
Vout / Vin ≈ 1 / (1 - d2)
```

## Mixed Operation

When both bridges participate:

```text
Vout / Vin = d1 / (1 - d2)
```

Vendor reference firmware uses a known-good mixed-mode allocation near `d1 ≈ 0.8` while varying the other leg. This is reference evidence only; the project’s unified allocator is free to choose a different realizable pair.

## Bidirectional Operation

The physical stage supports current in either direction. Port identities never swap:

```text
Port A = left physical terminal
Port B = right physical terminal
```

Reverse operation is represented by signed current/power and signed controller objectives, not by redefining the bridge mapping.

## Energy Storage and Loss Terms

The model hierarchy may add the following only when they materially improve prediction for the problem being studied:

- inductor DCR and saturation-dependent inductance;
- capacitor ESR/effective capacitance;
- MOSFET `RDS(on)` and temperature dependence;
- dead-time voltage error;
- switching loss;
- source/load impedance;
- sensing delay and filter dynamics.

A more complicated model is not automatically better; each added term must have traceable parameters and a measurable purpose.

## Dead Time and Commutation

Effective commutation is determined by HRTIM timing, Si8233 behavior, propagation mismatch, gate resistance, MOSFET charge, diode/body-diode conduction, parasitics, and operating point. `gate-drive-and-timing.md` owns the timing requirements.

## Model Hierarchy

The project may use:

1. switching model for commutation/ripple questions;
2. averaged large-signal model for estimator and nonlinear control;
3. operating-point small-signal model for local loop analysis;
4. discrete-time state-space model for digital control;
5. uncertainty/identified corrections when data justifies them.

## Validation Boundary

Do not perform a broad campaign to prove that the vendor power stage can Buck, Boost, or operate bidirectionally; that capability is already established.

Measure only what a new implementation needs, for example:

- actual acquisition/actuation timing;
- model parameters that materially affect `iL_hat`;
- new modulation transition behavior;
- effective constraints needed for safe HRTIM operation;
- controller-specific transient metrics.

Any development-only external inductor-current probe is validation instrumentation, not part of the final sensing architecture.
