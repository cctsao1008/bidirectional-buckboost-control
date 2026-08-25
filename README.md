# ⚡ Bidirectional Buck-Boost Control

A digital-power research platform for a four-switch, non-isolated, bidirectional buck-boost converter based on the STM32F334.

The project starts from a vendor-proven power stage and focuses on the control architecture that remains technically interesting after basic converter operation is already known to work: synchronized measurement, inductor-current state estimation without a permanent `iL` sensor, unified bidirectional control, and continuous constrained duty allocation.

> **Validate the implementation delta, not the vendor-proven baseline.**

---

## ✨ Why This Is Interesting

A four-switch bidirectional buck-boost converter is not difficult because Buck, Boost, or Mixed operation is unknown. Those operating modes are well understood, and the hardware is already known to regulate power successfully.

The interesting question is whether the converter really needs to be controlled as three separate operating regions.

This project explores a unified physical-state architecture that:

- keeps the physical ports fixed;
- represents power-flow direction with signed quantities;
- estimates the main-inductor current without adding a permanent `iL` sensor;
- lets the controller request average inductor voltage `vL*` rather than a mode-specific duty;
- exploits the redundant four-switch duty space through a continuous constrained `e1/e2` allocator.

This turns several practical implementation problems into one coherent control problem:

```text
mode switching
      ↓
continuous state-space behavior

missing iL sensor
      ↓
state estimation

multiple valid duty pairs
      ↓
constrained allocation

forward / reverse operation
      ↓
signed physical states
```

The target control path is:

```text
PWM-synchronized sensing
        ↓
Vin / Iin / Vout / Iout
        ↓
physics-based fast iL predictor
        ↓
slow measurement-based correction
        ↓
iL_hat
        ↓
voltage / current control
        ↓
vL*
        ↓
continuous constrained allocator
        ↓
e1 / e2
        ↓
d1 / d2
        ↓
HRTIM
        ↓
four-switch power stage
```

The research value is not in proving that a bidirectional buck-boost converter works. It is in testing whether the same power stage can be controlled with a simpler, continuous, and physically unified architecture.

---

## 🔌 Hardware Platform

The target board is the CBB024D / CBB02405D V1.2 four-switch synchronous bidirectional buck-boost converter.

![Four-switch bidirectional buck-boost topology](docs/images/four-switch-bidirectional-buck-boost-topology.svg)

### Switch mapping

```text
Left leg:
Q1 = high-side
Q4 = low-side

Right leg:
Q2 = high-side
Q3 = low-side
```

| Function | Device / MCU pin |
| --- | --- |
| Left high-side | Q1 / PA8 `PWM1H` |
| Left low-side | Q4 / PA9 `PWM1L` |
| Right high-side | Q2 / PA10 `PWM2H` |
| Right low-side | Q3 / PA11 `PWM2L` |

### Measurement mapping

| Signal | MCU pin |
| --- | --- |
| `Vin` | PA0 |
| `Iin` | PA1 |
| `Vout` | PA2 |
| `Iout` | PA3 |
| `VADJ` | PA4 |

The four main control measurements are routed through ADC1 sequential conversion, so sample order and per-channel timing are part of the measurement model rather than assumed simultaneous.

The board measures terminal currents through dedicated input/output shunts; it does not contain a permanent main-inductor-current shunt in the power path.

### Nominal characteristics

| Parameter | Value |
| --- | --- |
| MCU | STM32F334C8T6 |
| Switching rate | 200 kHz |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Suggested maximum power | 200 W |
| Main inductor | 22 µH nominal |
| Terminal-current shunts | R7 / R8, 1 mΩ |
| Main MOSFETs | BSC070N10NS3G |
| Gate drivers | Si8233BD-D-IS |
| Host UART | USART1, PB6 / PB7 |

---

## 🧱 Canonical Converter Model

Physical ports are fixed:

```text
Port A = physical left / schematic VIN side
Port B = physical right / schematic VOUT side
```

Project sign and duty conventions:

```text
Iin  > 0 : current enters the converter from Port A
Iout > 0 : current leaves the converter into Port B
iL   > 0 : inductor current flows left → right

d1 = Q1 left-leg high-side duty
d2 = Q3 right-leg low-side duty
```

Effective-duty coordinates are therefore:

```text
e1 = d1 = Q1 left-leg high-side effective duty
e2 = 1 - d2 = Q2 right-leg high-side effective duty
```

The CCM averaged model is:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

The controller requests average inductor voltage:

```text
d1 Vin - (1 - d2) Vout = vL*
```

The initial control scope is continuous-conduction operation. Buck, Mixed, and Boost remain useful descriptions of operating points, but they are not explicit control-architecture states.

---

## 🎛️ Effective-Duty Coordinates

The allocator uses effective duty coordinates:

```text
e1 = d1
e2 = 1 - d2
```

so the inductor-voltage relation becomes:

```text
Vin e1 - Vout e2 = vL*
```

or, in vector form,

```text
a = [ Vin, -Vout ]^T
e = [ e1,  e2   ]^T

a^T e = vL*
```

For one requested `vL*`, the feasible solutions form a line in the `(e1,e2)` plane. Duty, minimum-pulse, and bootstrap limits reduce the admissible region to a bounded box.

The allocator therefore solves a deterministic geometric problem:

```text
requested vL*
      ↓
feasibility clamp
      ↓
project previous (e1,e2)
onto the feasible line segment
      ↓
e1 / e2
      ↓
d1 = e1
d2 = 1 - e2
```

The initial secondary objective is minimum command movement:

```text
minimize
    (e1 - e1_prev)^2 + (e2 - e2_prev)^2

subject to
    Vin e1 - Vout e2 = vL*
    e1_min <= e1 <= e1_max
    e2_min <= e2 <= e2_max
```

This produces a continuous, constant-time allocator without explicit Buck/Mixed/Boost branching.

The null-space direction is:

```text
u = [ Vout, Vin ]^T
```

because:

```text
[ Vin, -Vout ] · [ Vout, Vin ] = 0
```

Movement along this direction redistributes the two effective duties without changing the requested average inductor voltage. The baseline implementation uses this redundant degree of freedom only to preserve continuity and minimize duty movement.

---

## 🧠 Inductor-Current Estimation

The board measures terminal currents but does not provide a dedicated ADC channel for main-inductor current.

The estimator therefore uses a physics predictor as the fast path:

```text
vL_realized
      ↓
physics predictor
      ↓
iL_pred
```

with a slower measurement-based correction path:

```text
Vin / Iin / Vout / Iout
          ↓
conditioned low-bandwidth correction
          ↓
iL_hat
```

The fast predictor is driven by the **realized** allocator output rather than the unconstrained controller request:

```text
vL_realized = Vin e1 - Vout e2
```

This keeps the estimator consistent with the duty commands actually sent to the power stage, including allocator saturation.

Terminal-current algebraic relationships are treated as correction information, not as per-cycle ground truth. Their usefulness depends on duty conditioning, capacitor-current dynamics, ADC noise, and sequential-sampling skew.

A temporary external inductor-current measurement may be used during development to validate `iL_hat`; it is not part of the final control architecture.

---

## 🧪 Four-Phase Development Path

Basic implementation plumbing such as startup GPIO state, UART, HRTIM setup, ADC/DMA drivers, protocol support, CI, and minimum shutdown infrastructure is required work but is not treated as a research result.

### Phase 1 — Synchronized Measurement

Establish deterministic measurement timing and capture:

```text
HRTIM trigger
    ↓
ADC1 scan + DMA
    ↓
coherent raw frame
    ↓
signed calibrated measurements
    ↓
indexed capture
```

Key outputs are known sample timing, quantified channel skew/noise, repeatable current offsets, and reliable signed `Vin/Iin/Vout/Iout` data.

### Phase 2 — Inductor-Current State Estimation

Implement and validate:

```text
fast physics predictor
        +
slow conditioned correction
        ↓
iL_hat
```

Validation uses synchronized converter data and a development-only external `iL` reference where required.

### Phase 3 — Unified Bidirectional Control

Build one cascaded control path around the estimated inductor current:

```text
voltage reference
      ↓
voltage controller
      ↓
iL_ref
      ↓
current controller
      ↓
vL*
```

The same control semantics are retained for A → B and B → A power flow. Direction is represented by signed current, power, and references rather than by remapping physical ports or timer ownership.

### Phase 4 — Continuous Allocation & Validation

Implement the `e1/e2` line-segment allocator, enforce hard duty constraints, and validate continuous operation across the intended CCM voltage-ratio and power-flow envelope.

The phase is complete when controller output, allocator behavior, realized `vL`, duty continuity, saturation handling, and direction reversal are experimentally coherent on the real converter.

---

## 🛡️ Safety Boundary

The converter operates with significant voltage, current, switching energy, and stored energy. Control development therefore assumes staged, low-energy bring-up before broader closed-loop testing.

Before energized switching, the implementation must provide at least:

```text
safe GPIO startup state
        ↓
HRTIM configured forced inactive
        ↓
safe GPIO → alternate-function handoff
        ↓
bounded duty / minimum-pulse limits
        ↓
Power Manager enable authority
        ↓
fault / shutdown authority over PWM
```

The current board has two important hardware limitations:

- the Si8233 gate-driver `DISABLE` input is not MCU-controlled;
- the existing current-sense outputs are not directly routed to STM32F334 comparator inputs for a hardware-speed overcurrent path.

These constraints are treated as real hardware limits during experiment planning. Software protection and HRTIM shutdown do not substitute for an unavailable independent analog fault path.

Do not rely on online SWD debugging while the power stage is energized. Halting the MCU can leave PWM-related hardware in an unsafe condition. High-side gate and switching-node measurements require appropriate differential or isolated instrumentation.

---

## 📚 Documentation

[`docs/design/README.md`](docs/design/README.md) defines the design-document source-of-truth hierarchy.

Key specifications:

- [`hardware-specification.md`](docs/design/hardware-specification.md) — physical board facts
- [`control-conventions.md`](docs/design/control-conventions.md) — fixed ports, signs, power, and duty definitions
- [`sensing-and-scaling.md`](docs/design/sensing-and-scaling.md) — measurement conversion and calibration
- [`current-observability-and-estimation.md`](docs/design/current-observability-and-estimation.md) — `iL_hat` model and validation
- [`modulation-and-operating-regions.md`](docs/design/modulation-and-operating-regions.md) — duty realization and constraints
- [`protection-and-state-machine.md`](docs/design/protection-and-state-machine.md) — Power Manager and shutdown policy
- [`host-interface-and-uart-protocol.md`](docs/design/host-interface-and-uart-protocol.md) — host wire protocol

Current progress is tracked through GitHub Issues and commits rather than embedded in stable architecture documentation.
