# Modulation and Operating Regions

## Purpose

This document defines the operating-region abstraction used by the control stack. The objective is to separate control-law design from the low-level switching details of the four-switch power stage.

The initial implementation follows the behavior of the known-good reference system closely enough to establish a reproducible baseline, while leaving room for later controllers to use different duty allocation or mode scheduling strategies.

## Power-Stage Abstraction

The converter consists of two synchronous half bridges connected by a single inductor:

- Left half-bridge: Q1 high-side / Q4 low-side, associated with duty variable `D1`
- Right half-bridge: Q2 high-side / Q3 low-side, associated with duty variable `D2`

This mapping follows the V1.2 schematic and MCU PWM routing.

The same hardware supports energy flow in either direction. Forward and reverse operation should therefore be expressed through a common power-flow abstraction rather than duplicated controller implementations.

## Reference Forward Operating Regions

The reference implementation divides the forward operating range into three regions:

| Condition | Region | Primary action |
| --- | --- | --- |
| `Vout < 0.8 × Vin` | Buck | Left half-bridge modulates; right half-bridge is held near the pass-through/non-switching state required by the hardware implementation |
| `0.8 × Vin ≤ Vout ≤ 1.2 × Vin` | Mixed buck-boost | Both half bridges participate |
| `Vout > 1.2 × Vin` | Boost | Right half-bridge modulates; left half-bridge is held near the corresponding pass-through/non-switching state |

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
Normalized or physical control request
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
enable_left_leg
enable_right_leg
```

The exact firmware API is intentionally deferred until the first independent implementation is exercised on hardware.

## Physical Duty Constraints

The modulation layer must enforce hardware-realizable PWM commands, not only ideal duty equations.

Constraints include:

- minimum and maximum duty ratio;
- minimum pulse width;
- effective dead time;
- bootstrap-refresh requirements;
- bounded duty slew during mode transitions;
- safe complementary-output states;
- protection authority over all controller requests.

A mathematically valid `(D1, D2)` pair is not automatically a safe or realizable gate command.

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

## Inductor-Voltage-Oriented Control Allocation

Later control methods may command a desired average inductor voltage rather than directly commanding a region-specific duty ratio.

For the ideal averaged model:

```text
vL = D1 * Vin - (1 - D2) * Vout
```

A controller may therefore request `vL*`, while the modulation layer chooses a realizable `(D1, D2)` pair that satisfies:

```text
D1 * Vin - (1 - D2) * Vout ≈ vL*
```

This separates the control-law objective from the implementation-specific choice of Buck, Mixed, or Boost duty allocation.

The remaining degree of freedom may later be used to reduce switching activity, conduction loss, duty discontinuity, or bootstrap stress, subject to measured hardware constraints.

## Bidirectional Operation

Forward and reverse power flow should share the same conceptual structure:

```text
requested power-flow direction
        ↓
voltage relationship between the two ports
        ↓
operating region / control allocation
        ↓
duty allocation
        ↓
PWM state
```

The implementation should avoid encoding `input` and `output` as permanent hardware identities wherever the underlying quantity is physically a bidirectional port variable.

This distinction becomes important for later state-space, LQI, observer-based, sliding-mode, predictive, and MPC implementations.

## Design Rules

1. Physical bridge mapping follows the V1.2 schematic: Q1/Q4 left, Q2/Q3 right.
2. Region boundaries are configuration parameters, not hidden constants.
3. Control algorithms do not write PWM registers directly.
4. Duty commands are bounded before reaching the hardware layer.
5. Region transitions are observable and testable events.
6. Bootstrap and minimum-pulse constraints belong to modulation, not individual controllers.
7. The reference modulation strategy is a baseline, not the final architecture.
8. Any new modulation strategy must be evaluated under the same measurement protocol as the baseline.

## Validation Targets

The operating-region implementation should eventually be validated using:

- `D1` / `D2` command traces;
- both switching-node waveforms;
- effective gate timing;
- reconstructed or externally observed inductor-current behavior during development;
- output voltage;
- transition overshoot / undershoot;
- transition current stress;
- transition time and repeatability.

These measurements are expected to determine whether the reference `0.8 × Vin` / `1.2 × Vin` boundaries remain appropriate for the independent control stack.
