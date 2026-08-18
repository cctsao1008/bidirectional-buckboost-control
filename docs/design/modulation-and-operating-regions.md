# Modulation and Operating Regions

## Purpose

This document defines the operating-region abstraction used by the control stack. The objective is to separate control-law design from the low-level switching details of the four-switch power stage.

The initial implementation follows the behavior of the known-good reference system closely enough to establish a reproducible baseline, while leaving room for later controllers to use different duty allocation or mode scheduling strategies.

## Power-Stage Abstraction

The converter consists of two synchronous half bridges connected by a single inductor:

- Half-bridge A: Q1 / Q2, duty variable `D1`
- Half-bridge B: Q4 / Q3, duty variable `D2`

The same hardware supports energy flow in either direction. Forward and reverse operation should therefore be expressed through a common power-flow abstraction rather than duplicated controller implementations.

## Reference Forward Operating Regions

The reference implementation divides the forward operating range into three regions:

| Condition | Region | Primary action |
| --- | --- | --- |
| `Vout < 0.8 × Vin` | Buck | Half-bridge A modulates; half-bridge B is held near the non-switching state required by the driver/bootstrap implementation |
| `0.8 × Vin ≤ Vout ≤ 1.2 × Vin` | Mixed buck-boost | Both half bridges participate |
| `Vout > 1.2 × Vin` | Boost | Half-bridge B modulates; half-bridge A is held near the corresponding non-switching state |

The `0.8` and `1.2` boundaries are reference-system values, not universal converter constants. They should remain explicit configuration parameters so that later experiments can evaluate different transition regions.

## Reference Mixed-Mode Duty Allocation

In the reference mixed-mode strategy, the buck-side duty is held near:

```text
D1 = 0.8
```

while the boost-side duty `D2` is varied.

For the idealized cascaded buck/boost interpretation:

```text
Vout / Vin = D1 / (1 - D2)
```

This duty allocation is useful as a baseline because it reduces mixed-mode regulation to a single active control variable. It is not treated as a requirement for later control methods.

## Unified Command Interface

The control stack should expose a normalized modulation request rather than allow each controller to manipulate timer compare registers directly.

Conceptually:

```text
Controller
    ↓
Normalized control command
    ↓
Operating-region scheduler
    ↓
Duty allocation / switching-state selection
    ↓
PWM implementation
    ↓
Gate driver
```

A future interface may contain fields such as:

```text
power_flow_direction
operating_region
D1
D2
enable_leg_A
enable_leg_B
```

The exact firmware API is intentionally deferred until the first independent implementation is exercised on hardware.

## Transition Design

Mode transitions are part of the controlled system and must not be treated as simple `if/else` changes in duty assignment.

A transition may involve:

- a different duty-to-output-voltage relationship;
- different switching-node waveforms;
- different current ripple;
- different effective plant dynamics;
- different bootstrap refresh constraints;
- different controller gains or state-space models.

The transition logic should therefore provide:

- hysteresis or another anti-chatter mechanism;
- bounded duty changes;
- explicit ownership of integrator/state transfer;
- deterministic PWM reconfiguration;
- protection continuity during the transition.

The final transition policy will be based on measured behavior rather than assumed from the ideal topology alone.

## Bidirectional Operation

Forward and reverse power flow should share the same conceptual structure:

```text
requested power-flow direction
        ↓
voltage relationship between the two ports
        ↓
operating region
        ↓
duty allocation
        ↓
PWM state
```

The implementation should avoid encoding `input` and `output` as permanent hardware identities wherever the underlying quantity is physically a bidirectional port variable.

This distinction becomes important for later state-space, LQR, LQG, gain-scheduled, and MPC implementations.

## Design Rules

1. Region boundaries are configuration parameters, not hidden constants.
2. Control algorithms do not write PWM registers directly.
3. Duty commands are bounded before reaching the hardware layer.
4. Region transitions are observable and testable events.
5. The reference modulation strategy is a baseline, not the final architecture.
6. Any new modulation strategy must be evaluated under the same measurement protocol as the baseline.

## Validation Targets

The operating-region implementation should eventually be validated using:

- `D1` / `D2` command traces;
- both switching-node waveforms;
- effective gate timing;
- inductor current;
- output voltage;
- transition overshoot / undershoot;
- transition current stress;
- transition time and repeatability.

These measurements are expected to determine whether the reference `0.8 × Vin` / `1.2 × Vin` boundaries remain appropriate for the independent control stack.