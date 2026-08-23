# Vendor Architecture vs Project Architecture

## Purpose

This document explains why this project exists even though the CBB024D / CBB02405D power stage already has a proven vendor reference implementation.

The comparison is architectural, not a claim that the project implementation is already better than the vendor implementation.

The vendor reference architecture demonstrates that the hardware can operate as a bidirectional four-switch buck-boost converter. This project starts from that evidence and investigates a different control abstraction.

> **Vendor architecture:** solve the converter as a set of known operating modes.
>
> **Project architecture:** solve the converter as one continuous physical system with a common state, control coordinate, and constrained duty allocator.

---

## 1. Architecture Overview

### Vendor reference architecture

```text
                 regulated quantity
                        │
                        ▼
                 operating-mode logic
                        │
          ┌─────────────┼─────────────┐
          ▼             ▼             ▼
        Buck          Mixed         Boost
       control       control       control
          │             │             │
      mode duty     mode duties    mode duty
          └─────────────┼─────────────┘
                        ▼
                      HRTIM
                        │
                        ▼
                 four-switch stage
```

The vendor examples treat Buck, Mixed, and Boost as explicit operating regions with region-dependent duty realization and control behavior. Region selection is therefore part of the control architecture.

### Project target architecture

```text
              Vin / Iin / Vout / Iout
                         │
                         ▼
              physics-based iL predictor
                         │
                         ▼
             slow measurement correction
                         │
                         ▼
                       iL_hat
                         │
                         ▼
                voltage/current control
                         │
                        vL*
                         │
                         ▼
            continuous constrained allocator
                         │
                       e1/e2
                         │
                 d1=e1, d2=1-e2
                         │
                         ▼
                       HRTIM
                         │
                         ▼
                 four-switch stage
```

The project does not use Buck, Mixed, and Boost as separate controller identities. They remain useful descriptions of operating points, but the controller and physical-port definitions remain unchanged across the intended CCM operating envelope.

---

## 2. Core Abstraction

The main difference is the variable used to describe what the controller wants from the power stage.

### Vendor architecture: operating mode first

The vendor reference behavior can be summarized as:

```text
voltage/current error
        ↓
determine operating region
        ↓
apply region-specific control/duty behavior
        ↓
PWM duty commands
```

The operating-region decision is therefore structurally important. Buck, Mixed, and Boost determine which duty is actively regulated, which duty is held near a known value, and how transitions are handled.

This is a practical architecture. It is easy to understand, easy to tune region by region, and already demonstrated on the physical board.

### Project architecture: physical actuation first

The project instead starts from the CCM averaged inductor equation:

```text
L diL/dt = d1 Vin - (1 - d2) Vout
```

and defines the requested average inductor voltage:

```text
vL*
```

The controller therefore asks for a physical actuation quantity rather than a mode-specific duty.

The target relationship is:

```text
d1 Vin - (1 - d2) Vout = vL*
```

The allocator, not the controller, decides how the two switching legs realize that request.

---

## 3. Effective-Duty Coordinates

The project introduces:

```text
e1 = d1
e2 = 1 - d2
```

which gives:

```text
Vin e1 - Vout e2 = vL*
```

For fixed `Vin`, `Vout`, and `vL*`, the valid solutions form a line in the `(e1,e2)` plane. Hardware constraints reduce the allowable duty space to a bounded box.

The baseline allocator solves:

```text
minimize
    (e1 - e1_prev)^2 + (e2 - e2_prev)^2

subject to
    Vin e1 - Vout e2 = vL*
    e1_min <= e1 <= e1_max
    e2_min <= e2 <= e2_max
```

This converts the mode-transition problem into a continuous constrained-allocation problem.

The redundant direction:

```text
u = [ Vout, Vin ]^T
```

lies in the null space of:

```text
[ Vin, -Vout ]
```

so movement along `u` redistributes the two effective duties without changing the requested ideal average inductor voltage.

The initial architecture uses that redundant degree of freedom only to preserve continuity and minimize duty movement.

---

## 4. Buck / Mixed / Boost Interpretation

### Vendor architecture

Buck, Mixed, and Boost are explicit architectural regions.

A simplified interpretation is:

```text
Buck-like region
    → one leg primarily performs regulation

Mixed region
    → both legs participate according to a defined mixed-mode policy

Boost-like region
    → the opposite leg primarily performs regulation
```

The vendor material provides known-good region thresholds and mixed-mode duty behavior. These are useful reference evidence for the physical board.

### Project architecture

The project does not require an explicit:

```c
switch (mode) {
case BUCK:
case MIXED:
case BOOST:
}
```

in the core control path.

Instead:

```text
requested vL*
     ↓
feasible e1/e2 line
     ↓
constraint-aware projection
     ↓
continuous e1/e2 trajectory
```

A Buck-like, Mixed-like, or Boost-like operating point emerges from which constraints are active and where the solution lies in duty space.

The labels remain useful for plots, validation, and discussion, but not as separate controller identities.

---

## 5. Transition Handling

### Vendor architecture

An explicit mode scheduler creates explicit mode transitions. The reference implementation therefore requires transition-management behavior when moving between Buck, Mixed, and Boost control regions.

This is a reasonable engineering solution because each region has its own known-good control realization.

### Project architecture

The target allocator is continuous in its control coordinates. As `Vin`, `Vout`, and `vL*` vary, the feasible line and its projected solution move continuously, subject to hard saturation.

The intended behavior is therefore:

```text
voltage ratio changes
        ↓
feasible set moves
        ↓
e1/e2 solution moves continuously
        ↓
no controller identity change
```

The project must still validate duty continuity, constraint activation, saturation, and real switching behavior experimentally. Continuous mathematics does not by itself prove a disturbance-free hardware transition.

---

## 6. Current Feedback and State Representation

### Vendor architecture

The vendor examples regulate measured terminal quantities directly using conventional voltage/current control structures appropriate to their operating modes.

They do not require a unified reconstructed main-inductor-current state as the central control variable.

### Project architecture

The project treats main-inductor current as the common energy-transfer state:

```text
vL_realized
      ↓
physics predictor
      ↓
iL_pred
      ↓
slow measurement correction
      ↓
iL_hat
```

The fast predictor uses:

```text
vL_realized = Vin e1 - Vout e2
```

rather than the unconstrained controller request, so the estimator follows the actuation actually realized by the allocator.

Because the board has no permanent direct `iL` ADC channel, terminal-current relationships are used only as conditioned, low-bandwidth correction information rather than per-cycle ground truth.

A temporary external `iL` measurement is allowed during development to falsify or validate the estimator.

---

## 7. Bidirectional Semantics

### Vendor reference behavior

The vendor examples demonstrate both forward and reverse operation, but their implementation material is organized around operating modes and direction-specific application behavior.

### Project architecture

The project fixes physical meaning permanently:

```text
Port A = physical left / schematic VIN side
Port B = physical right / schematic VOUT side
```

and uses signed quantities:

```text
Iin  > 0 : current enters from Port A
Iout > 0 : current leaves into Port B
iL   > 0 : inductor current flows A → B
```

Reverse power flow does not rename ports, swap ADC channels, or change timer ownership.

The allocator itself does not need a power-direction branch. Direction is represented by the controlled state and references.

---

## 8. Measurement Architecture

This project deliberately preserves the proven hardware acquisition path where it is useful.

Both architectures use the existing board measurements:

```text
Vin
Iin
Vout
Iout
```

The project, however, treats ADC1 sequential conversion timing as part of the measurement model. The four values are not assumed simultaneous.

This matters because state estimation is more sensitive to sample timing, skew, offset, and noise than a conventional slow voltage-regulation loop.

Therefore the project adds explicit characterization of:

```text
PWM sample phase
ADC channel order
inter-channel skew
current offset
noise floor
capture timing
```

This is an implementation delta, not a rejection of the vendor acquisition concept.

---

## 9. Safety and Supervisory Boundary

The project does not claim that a new control architecture removes the hardware limitations of the board.

The current board still has important constraints:

- Si8233 `DISABLE` is not MCU-controlled;
- the existing current-sense outputs are not directly routed to STM32F334 comparator inputs for a hardware-speed overcurrent path.

The project therefore separates:

```text
control request
      ↓
allocator constraints
      ↓
Power Manager authority
      ↓
HRTIM output authority
```

and uses staged, low-energy bring-up before broader closed-loop testing.

A future hardware revision may improve the protection boundary, but those changes are not part of the current control architecture itself.

---

## 10. Summary Comparison

| Item | Vendor Architecture | Project Architecture |
| --- | --- | --- |
| Core abstraction | Buck / Mixed / Boost operating modes | Physical state + `vL*` + continuous allocator |
| Controller organization | Region-dependent control behavior | One common CCM control path |
| Primary fast state | Measured regulated terminal quantities | Estimated main-inductor current `iL_hat` |
| Controller output | Mode-specific duty command / realization | Requested average inductor voltage `vL*` |
| Duty realization | Region-specific policy | Continuous constrained `e1/e2` projection |
| Region transition | Explicit transition management | Continuous duty-space trajectory, subject to saturation |
| Power direction | Demonstrated through direction/mode-specific reference behavior | Fixed ports + signed states/references |
| ADC use | Proven ADC1 scan + DMA reference behavior | Same hardware basis, with timing/skew modeled explicitly |
| Main purpose | Practical proven converter examples | Investigate a unified physical-state control architecture |

---

## 11. What This Project Is — and Is Not — Claiming

The project is **not** claiming:

```text
our architecture is already better than the vendor architecture
```

The project is testing a narrower and more defensible claim:

> The same four-switch hardware may be controlled through one continuous physical-state and duty-allocation architecture, without making Buck, Mixed, and Boost separate controller identities.

That claim must be supported by implementation and hardware evidence.

The vendor implementation remains the reference baseline for known-good board behavior. The project validates only the architectural delta required to test the unified approach.
