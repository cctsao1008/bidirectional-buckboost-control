# Modeling Strategy

## Purpose

This document defines how analytical, switching, averaged, small-signal, state-space, discrete-time, and identified models are used in the project.

Models exist to answer a control/estimation question. The project does not build or validate models merely to duplicate vendor demonstrations.

## Canonical Large-Signal Backbone

For estimation and unified control, the canonical ideal averaged state is:

```text
x = [ Vin, iL, Vout ]^T
```

with terminal currents and duties treated as measured/known inputs:

```text
u = [ Iin, Iout, d1, d2 ]
```

Using `control-conventions.md`:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

This three-state form is the canonical model for the `iL_hat` observer and unified `vL*` actuation architecture.

Reduced models are allowed when their assumptions are explicit. For example, if Port A is sufficiently stiff for a specific local controller design, `Vin` may be treated as a measured disturbance rather than a dynamic state. Such a reduction is not the project-wide definition of the plant.

## Model Layers

```text
Switching model
    ↓
Averaged large-signal model
    ↓
Operating-point / small-signal model
    ↓
State-space controller/observer model
    ↓
Discrete-time implementation model
    ↓
Measured / identified correction
```

No layer replaces the others.

## Switching Model

Use when the question depends on actual bridge states or switching timing, for example:

- ripple and current slope;
- dead-time effects;
- minimum pulse constraints;
- ADC quiet-window selection;
- mode/allocation transitions;
- commutation stress.

The switching model should use the V1.2 physical mapping Q1/Q4 left and Q2/Q3 right.

## Averaged Model

Use for:

- energy flow;
- nonlinear state prediction;
- `iL_hat` estimation;
- `vL*`-oriented control design;
- control-allocation reasoning;
- large-signal operating-envelope studies.

The averaged model removes individual switching events but does not imply that PWM timing or sensing delay is irrelevant to the implemented controller.

## Operating-Point and Small-Signal Models

Linearize only around stated conditions. Every local model should record:

```text
Vin
Vout
Iin/Iout or load
power-flow direction
operating region
d1 / d2
switching frequency
controller/sample rate
parameter set
```

Small-signal models may support PI design, bandwidth/phase-margin analysis, local gain scheduling, or comparison with measured response. Vendor PI/PID/Type-III examples are reference evidence, not required project milestones.

## State-Space Models

General form:

```text
x_dot = A x + B u + E d
y     = C x + D u
```

The model must explicitly define:

- state vector;
- manipulated input (`vL*`, `d1/d2`, or another representation);
- measured disturbances;
- outputs;
- operating point and direction;
- whether modulation dynamics are included or abstracted.

State-space models support observability/controllability analysis, LQI, observers, Kalman methods, and MPC.

## Discrete-Time Implementation Model

The implementation operates with finite sample and actuation delay:

```text
x[k+1] = Ad x[k] + Bd u[k] + Ed d[k]
y[k]   = Cd x[k] + Dd u[k]
```

Every digital model used for controller implementation should specify:

```text
sample period
PWM update convention
ADC trigger phase
ADC channel sequence/latency
computation delay
actuation delay
zero-order hold / discretization method
```

Ignoring these details can invalidate an otherwise correct continuous-time design.

## Measurement Dynamics

The controller observes the plant through:

```text
physical quantity
    ↓
shunt/divider + op-amp
    ↓
analog RC network
    ↓
PWM-synchronous ADC sample
    ↓
scan/conversion latency
    ↓
DMA
    ↓
calibration/filtering
```

Measurement dynamics are added to a model only when significant relative to estimator/controller bandwidth.

## Parameter Provenance

Classify model parameters as:

### Schematic / nominal

Examples: L, C, resistor ratios, shunt value, gate resistor, nominal DCR.

### Datasheet-derived

Examples: MOSFET resistance/charge bounds, driver timing bounds, device tolerances.

### Measured / identified

Examples: effective L, effective C/ESR, current offset/gain, dead time, sensor delay, loss terms.

Measured values supersede nominal values only when method, operating condition, and dataset identity are recorded.

## Machine-Readable Parameters

The project should converge on a single machine-readable parameter source, for example:

```text
models/parameters/plant.yaml
```

It should contain engineering values plus provenance/confidence, not copies of third-party datasheets. Simulation, analysis, and firmware-generated configuration should derive from this source where practical.

## Identification Strategy

Only identify parameters that materially affect a current engineering question. Useful methods include:

- current-slope estimation for effective L;
- terminal-voltage/current transients for effective C/source/load dynamics;
- small perturbation response for local loop models;
- measured timing for ADC/HRTIM delay;
- loss correlation if control allocation optimizes efficiency.

Do not launch broad identification simply to re-prove the vendor baseline.

## Model Acceptance

A model is accepted for a stated purpose only when it records:

- intended operating envelope;
- state/input/output definitions;
- parameter set and provenance;
- validation dataset;
- quantitative error/fit metric appropriate to the control question;
- known limitations.

A model may be valid for one controller or operating region and inadequate elsewhere. That is an expected engineering result.

## Relationship to Controller Families

| Controller / function | Minimum useful model basis |
| --- | --- |
| Cascaded PI | local measured/small-signal dynamics + `iL_hat` quality |
| LQI | state-space model + observer/state definition |
| Deadbeat current control | low-delay discrete inductor model |
| ST-SMC | bounded plant/measurement uncertainty |
| MPC | discrete state-space model + explicit constraints |
| `iL_hat` estimator | canonical three-state large-signal/discrete model |
| Unified allocator | averaged `vL* = d1 Vin - (1-d2) Vout` relation + constraints |

## Design Rules

1. Use the canonical three-state backbone unless a documented reduction is justified.
2. Tie every model to operating conditions and direction.
3. Keep nominal and measured parameters distinct.
4. Include sensing/actuation delay when relevant.
5. Validate quantitatively against only the evidence required by the new design.
6. Do not hide mismatch by retuning without explaining the cause.
7. More complex control is allowed only when model/estimator quality and MCU timing can support it.
