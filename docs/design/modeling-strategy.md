# Modeling Strategy

## Purpose

This document defines how analytical, simulation, and identified models are used in the project.

The goal is not to maintain one abstract converter model and assume it is correct everywhere. The four-switch converter changes switching behavior and effective plant dynamics across buck, mixed, and boost operation, so models must be associated with operating conditions and validated against hardware.

## Modeling Layers

The project uses several model layers, each serving a different purpose:

```text
Switching model
    ↓
Averaged model
    ↓
Small-signal model
    ↓
State-space model
    ↓
Discrete-time implementation model
    ↓
Measured / identified correction
```

No single layer replaces the others.

## Switching Model

The switching model represents the actual bridge states, PWM timing, inductor, capacitors, and relevant parasitics.

It is useful for studying:

- switching-node behavior;
- inductor ripple;
- mode transitions;
- dead-time effects;
- current stress;
- ripple and commutation behavior.

The known-good reference simulations are useful comparison material, but independently created models should become the public, reproducible source for project analysis.

## Averaged Model

The averaged model removes individual switching events while preserving the dominant energy-storage dynamics.

Nominal state variables are expected to include at least:

```text
x = [ iL, vC ]ᵀ
```

with additional states introduced only when they materially improve prediction, for example sensor filtering or other dominant dynamics.

The averaged model is the starting point for operating-point analysis and many classical-control calculations.

## Operating-Point Models

Buck, mixed, and boost regions should not automatically be treated as one invariant linear plant.

For each relevant operating point, record at least:

- `Vin`;
- `Vout`;
- load or output current;
- power-flow direction;
- operating region;
- `D1` and `D2`;
- switching frequency;
- model parameters used.

This provides the basis for gain scheduling and for understanding where one controller design ceases to represent the plant adequately.

## Small-Signal Model

Linearization around a steady operating point provides transfer functions or equivalent state-space perturbation models for controller design.

Typical perturbation quantities may include:

```text
îL
v̂out
d̂1
d̂2
v̂in
îload
```

The exact input/output pairing depends on the controller being designed.

Small-signal models are expected to support:

- PI/PID tuning;
- Type-III compensation;
- loop-gain analysis;
- bandwidth and phase-margin studies;
- comparison with measured frequency response where practical.

## State-Space Model

A continuous-time representation has the general form:

```text
ẋ = A x + B u + E d
y  = C x + D u
```

where:

- `x` contains plant states;
- `u` contains manipulated inputs such as duty commands;
- `d` contains disturbances such as input-voltage or load variation;
- `y` contains measured or controlled outputs.

The model should explicitly state whether `u` contains one active duty variable or both `D1` and `D2`.

State-space models support:

- controllability and observability analysis;
- pole placement;
- LQR;
- observers;
- Kalman estimation / LQG;
- MPC;
- gain-scheduled control.

## Measurement Dynamics

The controller does not observe ideal plant states directly.

Voltage and current measurements pass through:

```text
physical quantity
    ↓
shunt / divider
    ↓
analog amplifier
    ↓
RC filtering
    ↓
ADC sample timing
    ↓
digital scaling / filtering
```

Where measurement dynamics are significant relative to control bandwidth, they should be included in the effective model or handled explicitly in estimator design.

## Discrete-Time Model

The implemented controller operates at a finite sample rate with finite computation and actuation delay.

A discrete representation may be written as:

```text
x[k+1] = Ad x[k] + Bd u[k] + Ed d[k]
y[k]   = Cd x[k] + Dd u[k]
```

The discretization must specify:

- sampling period;
- PWM update convention;
- ADC sample phase;
- computation delay;
- zero-order-hold or other discretization assumption.

A mathematically correct continuous controller can behave differently after discretization if these details are ignored.

## Parameter Sources

Model parameters should come from traceable sources and be classified by confidence:

### Nominal component data

Examples:

- inductance;
- inductor DCR;
- capacitance;
- shunt resistance;
- MOSFET resistance and charge data.

### Schematic-derived values

Examples:

- sensing ratios;
- amplifier gains;
- RC filters;
- gate resistance.

### Measured values

Examples:

- actual inductance;
- actual capacitor ESR;
- current-sense zero offset;
- effective dead time;
- sensor delay;
- power-stage loss.

Measured values should supersede nominal assumptions only when the measurement method and conditions are recorded.

## Parameter Database

Selected plant parameters should eventually be stored in a machine-readable source such as:

```text
models/parameters/plant.yaml
```

The intent is to avoid independent copies of the same constants in simulation scripts, controller design notebooks, test tools, and firmware documentation.

The parameter file should contain selected engineering values and provenance, not bulk copies of third-party datasheets.

## Identification Strategy

Model identification is used to close the gap between calculation and real hardware.

Possible methods include:

- steady-state duty / voltage correlation;
- inductor-current slope measurements;
- load-step response;
- small perturbation response;
- frequency-response measurement where instrumentation and operating conditions permit.

Identification is not a substitute for physical modeling. It is used to validate, refine, or reject model assumptions.

## Validation Loop

The project follows this loop:

```text
Choose operating point
        ↓
Predict behavior
        ↓
Run simulation
        ↓
Measure hardware
        ↓
Compare quantitative metrics
        ↓
Explain mismatch
        ↓
Update model or assumptions
        ↺
```

Useful comparison quantities include:

- steady-state conversion ratio;
- inductor-current ripple;
- transient shape;
- overshoot / undershoot;
- settling time;
- dominant frequency / damping;
- measured loop response where available.

## Model Acceptance

A model is not accepted because it produces plausible waveforms.

Every validated model should state:

- intended operating region;
- operating-point range;
- parameter set;
- measurement data used for validation;
- metrics used for comparison;
- known limitations.

A model may be valid for controller design around one operating point and invalid for another. That is an expected result, not necessarily a modeling failure.

## Relationship to Control Research

The modeling hierarchy maps naturally to the controller families in this project:

| Control method | Minimum useful model basis |
| --- | --- |
| PI / PID | measured dynamics or low-order small-signal model |
| Type-III | loop / small-signal frequency-domain model |
| State feedback | state-space model |
| LQR | state-space model and weighting definition |
| Observer | state-space model and measurement model |
| Kalman / LQG | discrete or continuous stochastic state-space model with noise assumptions |
| Gain scheduling | validated models across multiple operating points |
| MPC | discrete state-space model plus explicit constraints |

The purpose of advanced control is therefore inseparable from model quality.

## Design Rules

1. Every model is tied to operating conditions.
2. Nominal component values and measured values are not silently mixed.
3. Sensor and implementation dynamics are included when relevant to loop behavior.
4. Controller comparison uses the same physical plant and measurement convention.
5. Model validation is quantitative.
6. Mismatch is investigated rather than hidden by retuning.
7. More complex controllers are introduced only when the model is sufficiently validated to support them.