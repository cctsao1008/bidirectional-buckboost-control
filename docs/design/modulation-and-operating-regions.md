# Modulation and Operating Regions

## Purpose

This document defines the control-allocation and operating-region abstraction between controller output and physical HRTIM waveforms.

The project does **not** begin by reproducing the vendor mode scheduler. Vendor Buck/Mixed/Boost behavior is reference evidence. The project baseline is a unified actuation interface based on desired average inductor voltage.

All physical mapping and sign definitions follow `control-conventions.md`.

## Physical Power-Stage Mapping

```text
left bridge  : Q1 high / Q4 low
right bridge : Q2 high / Q3 low
```

Canonical duty variables:

```text
d1 = average on-time fraction of Q1, left high-side
d2 = average on-time fraction of Q3, right low-side
```

Ideal averaged inductor equation:

```text
vL = d1 Vin - (1 - d2) Vout
```

## Primary Modulation Interface

Controllers should request:

```text
vL*
```

not raw timer compare values and not a mode-specific duty variable.

The modulation layer solves:

```text
d1 Vin - (1 - d2) Vout ≈ vL*
```

subject to hardware and operating-policy constraints.

Conceptually:

```text
Controller
   ↓
vL*
   ↓
Control Allocation
   ↓
Operating-region / constraint policy
   ↓
d1 / d2
   ↓
Platform HRTIM API
```

This is the project’s baseline architecture, not a future optional extension.

## Redundant Degree of Freedom

For a requested `vL*`, many `(d1,d2)` pairs may satisfy the averaged equation. The remaining degree of freedom can be used to optimize engineering objectives such as:

- lower switching activity;
- lower estimated conduction/switching loss;
- smaller duty discontinuity from the previous cycle;
- adequate bootstrap refresh;
- larger numerical/physical margin from minimum pulse limits;
- smoother region transitions.

A future allocator may therefore minimize a cost such as:

```text
J = w_delta * ||d - d_previous||^2
  + w_loss  * estimated_loss
  + w_margin * constraint_penalty
```

subject to the `vL*` equality/approximation and hard safety constraints. This optimization must remain computationally bounded for the STM32F334.

## Operating Regions

Buck, Mixed, and Boost remain useful **modulation regions** because different duty allocations are efficient or physically convenient at different voltage ratios. They are not separate controller identities.

Vendor forward reference behavior uses approximately:

| Voltage relationship | Reference region |
| --- | --- |
| `Vout < 0.8 Vin` | Buck |
| `0.8 Vin <= Vout <= 1.2 Vin` | Mixed |
| `Vout > 1.2 Vin` | Boost |

and a mixed-mode strategy near:

```text
d1 ≈ 0.8
```

while varying the right-leg duty variable. These values are retained only as known-good reference points. The independent allocator may use different boundaries or a continuous allocation policy.

## Hardware Constraints

A mathematically valid duty pair is not automatically realizable. The modulation layer must enforce:

```text
0 <= d1 <= 1
0 <= d2 <= 1
minimum pulse width
minimum off-time
maximum practical duty
commanded/effective dead-time constraints
bootstrap refresh
complementary-output legality
fault-forced inactive state
```

Limits are based on the actual hardware/timing implementation, not hidden controller constants.

## Region Selection and Hysteresis

If discrete region labels are used, the scheduler must include anti-chatter behavior such as hysteresis and must expose region transitions as observable events.

A region change may alter switching activity, ripple, sensing noise, and bootstrap conditions even when the controller state is unchanged. Transition handling therefore belongs in modulation and must be deterministic.

## Bumpless Transition

A Buck/Mixed/Boost transition must preserve the controller’s physical actuation request as closely as possible:

```text
vL*_before ≈ vL*_after
```

while changing the duty allocation smoothly. The allocator should minimize unnecessary `d1` / `d2` discontinuity and return saturation information to the controller for anti-windup.

Controller integrators are not reset merely because the modulation region changes unless a specific controller design requires it.

## Bidirectional Operation

Power direction and operating region are separate dimensions. The same physical mapping and duty definitions apply in both directions.

```text
requested direction / signed references
        ↓
controller produces signed vL*
        ↓
unified allocator
        ↓
realizable d1 / d2
```

The implementation must not swap ADC channels or timer ownership to reverse power flow.

## DCM, Zero Crossing, and Light Load

The ideal CCM equation remains a useful average command model, but light-load/DCM behavior can invalidate assumptions about current continuity and synchronous conduction.

The modulation layer must eventually define:

- whether reverse current is allowed in each operating state;
- zero-current crossing policy;
- synchronous-rectification policy at light load;
- discontinuous-conduction handling;
- estimator-confidence requirements before current-dependent actuation.

These policies are part of Phase 3/4 work and are not inferred from vendor examples.

## HRTIM Boundary

The modulation layer outputs logical duties/state requests. Only the platform layer converts those requests to compare-register values and complementary output configuration.

```text
modulation_request_t
  d1
  d2
  enable request
  saturation / validity metadata
        ↓
platform PWM implementation
```

The exact C API can evolve, but the ownership boundary may not.

## Validation of New Modulation

Validate the implementation delta using:

- requested `vL*` versus realized average actuation;
- `d1` / `d2` continuity;
- HRTIM/gate legality;
- transition overshoot/current stress;
- estimator confidence across transitions;
- bootstrap/minimum-pulse margin;
- execution time.

Do not repeat vendor Buck/Boost/Mixed characterization as a standalone milestone.

## Design Rules

1. V1.2 physical mapping is Q1/Q4 left and Q2/Q3 right.
2. Project notation is lower-case `d1` / `d2`.
3. Controllers request `vL*`; modulation owns duty allocation.
4. Region labels describe switching/allocation behavior, not different controller laws.
5. Hard hardware constraints are enforced below controllers.
6. Direction changes do not remap hardware channels.
7. Region transitions are bumpless by design and observable in telemetry/capture.
8. Vendor 0.8/1.2 boundaries are reference values, not architecture constants.
