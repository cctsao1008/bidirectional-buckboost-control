# Modeling Architecture

## Purpose

This document defines the model hierarchy, canonical state definitions, timing semantics, and parameter classes used by control and estimation.

## Canonical large-signal model

The project-wide averaged state is:

```text
x = [ Vin, iL, Vout ]^T
```

with measured/known inputs:

```text
u = [ Iin, Iout, d1, d2 ]
```

The ideal averaged equations are:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

Equivalent effective-duty form:

```text
e1 = d1
e2 = 1 - d2

L diL/dt = Vin e1 - Vout e2
```

This is the common plant backbone for state estimation and `vL*` actuation.

## Model layers

```text
Switching model
    ↓
Averaged large-signal model
    ↓
Operating-point / small-signal model
    ↓
State-space controller / observer model
    ↓
Discrete-time implementation model
```

Each layer answers a different control-system question.

### Switching model

Represents bridge state and switching timing for:

- current ripple and slope;
- dead-time behavior;
- minimum-pulse constraints;
- ADC quiet-window placement;
- commutation and gate timing.

### Averaged model

Represents energy flow and nonlinear state evolution for:

- `iL_hat` prediction;
- `vL*` control;
- control allocation;
- operating-envelope analysis.

### Small-signal model

A local linearization is defined together with its operating point:

```text
Vin
Vout
Iin / Iout or load
power-flow direction
d1 / d2
switching frequency
sample/control rate
parameter set
```

### State-space model

General form:

```text
x_dot = A x + B u + E d
y     = C x + D u
```

Every state-space model defines its state vector, manipulated input, measured disturbances, outputs, operating point, and modulation abstraction.

### Discrete-time model

```text
x[k+1] = Ad x[k] + Bd u[k] + Ed d[k]
y[k]   = Cd x[k] + Dd u[k]
```

Digital-control models include:

```text
sample period
PWM update convention
ADC trigger phase
ADC channel sequence / latency
computation delay
actuation delay
discretization / hold assumption
```

## Measurement dynamics

The controller observes the plant through:

```text
physical quantity
    ↓
shunt / divider + op-amp
    ↓
analog RC network
    ↓
PWM-synchronous ADC sample
    ↓
scan / conversion latency
    ↓
DMA
    ↓
calibration / filtering
```

Measurement dynamics are included when their delay or attenuation is material relative to estimator/controller bandwidth.

## Parameter classes

### Schematic / nominal

Examples: `L`, `C`, resistor ratios, shunt value, gate resistor.

### Datasheet-derived

Examples: MOSFET resistance/charge, driver timing bounds, component tolerance.

### Calibrated / measured

Examples: current gain/offset, voltage gain/offset, effective inductance, effective capacitance/ESR, dead time, sensor delay, loss terms.

A measured parameter includes operating condition and measurement identity. Nominal and measured values remain distinguishable.

## Model validity

Every model used by control or estimation has a stated:

```text
operating envelope
state / input / output definition
parameter set
sample and actuation timing
known constraints
validation metric
```

A reduced model is valid only inside the assumptions stated for that model. Treating Port A as a stiff source, for example, is a local reduction and does not redefine the project-wide plant state.

## Controller/model mapping

| Function | Model basis |
| --- | --- |
| Cascaded PI | local dynamics + `iL_hat` quality |
| LQI | state-space model + observer/state definition |
| Deadbeat current control | low-delay discrete inductor model |
| Super-Twisting SMC | bounded plant/measurement uncertainty |
| MPC | discrete state-space model + explicit constraints |
| `iL_hat` estimator | three-state large-signal/discrete model |
| Unified allocator | `vL* = Vin e1 - Vout e2` + hardware constraints |

## Design rules

1. The three-state model is the canonical large-signal backbone.
2. Port and current signs follow `control-conventions.md`.
3. Sensing and actuation timing are part of the digital model.
4. Nominal and measured parameters are not silently merged.
5. Added model complexity requires a defined physical parameter and measurable effect.
6. Controller validity is limited to the model and estimator envelope on which it depends.