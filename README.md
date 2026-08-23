# Bidirectional Buck-Boost Control

A digital-power research platform for a four-switch, non-isolated, bidirectional buck-boost converter based on the STM32F334.

The project starts from a vendor-proven 200 W power stage and focuses on the control problems that remain interesting after basic converter operation is already known to work: synchronized sensing, state estimation without a dedicated inductor-current ADC, unified bidirectional control, advanced model-based control, optimization-assisted design, and learning-enhanced control.

> **Validate the implementation delta, not the vendor-proven baseline.**

---

## Project Direction

The goal is not to reproduce another Buck / Boost / PID demonstration.

The goal is to build one coherent control architecture around the physical converter:

```text
PWM-synchronized sensing
        ↓
Vin / Iin / Vout / Iout
        ↓
state estimation
        ↓
iL_hat
        ↓
voltage / energy control
        ↓
iL_ref
        ↓
current control
        ↓
vL*
        ↓
unified constrained allocation
        ↓
d1 / d2
        ↓
HRTIM
        ↓
four-switch power stage
```

The canonical averaged relation is:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

and the control abstraction is:

```text
d1 Vin - (1 - d2) Vout = vL*
```

This deliberately separates the controller from the switching-region details. Buck, Mixed, and Boost behavior are handled by the allocation/modulation layer rather than by three unrelated control laws.

---

## Four-Phase Research Roadmap

Basic firmware bring-up, UART, GPIO initialization, HRTIM setup, ADC/DMA drivers, protocol support, CI, and minimum protection plumbing are implementation prerequisites. They are necessary, but they are not counted as research phases.

### Phase 1 — Measurement & State Estimation

Build the measurement foundation required by every later control method.

```text
PWM-synchronized ADC
        ↓
signed calibration
        ↓
Vin / Iin / Vout / Iout
        ↓
physics-based state estimator
        ↓
iL_hat + validity/confidence
```

Primary work:

- HRTIM-synchronized ADC acquisition;
- deterministic sample phase and conversion timing;
- signed current calibration and zero-current offset handling;
- timestamped / indexed data capture;
- inductor-current observability analysis;
- physics-based `iL` reconstruction using existing sensors;
- development-only external `iL` reference measurement where needed to validate the estimator.

The final architecture does **not** add a dedicated inductor-current sensor.

### Phase 2 — Unified Bidirectional Control

Create one control stack for the full converter operating envelope.

```text
iL_hat
   ↓
voltage / energy controller
   ↓
iL_ref
   ↓
current controller
   ↓
vL*
   ↓
unified d1/d2 allocation
   ↓
Buck / Mixed / Boost
   ↓
A ↔ B power flow
```

Primary work:

- cascaded voltage/current control baseline;
- `vL*` as the controller-to-modulator interface;
- constrained `d1/d2` allocation;
- minimum-pulse, dead-time, and bootstrap constraints;
- smooth Buck / Mixed / Boost transitions;
- fixed physical Port A / Port B semantics;
- signed current and signed power-flow representation;
- controlled forward/reverse and zero-current transitions;
- one firmware image for both power-flow directions.

Completion of Phase 2 establishes the core project: a unified bidirectional digital-power platform independent of vendor mode-specific control structure.

### Phase 3 — Advanced Control & Optimization

Introduce more capable control methods only where they provide a useful engineering benefit.

Candidate methods include:

- LQI;
- Deadbeat Predictive Current Control;
- Super-Twisting Sliding Mode Control;
- constrained / reduced-complexity MPC.

Optimization is treated as a design tool rather than a project goal by itself. Candidate optimizers may include:

- Genetic Algorithm (GA);
- Particle Swarm Optimization (PSO);
- CMA-ES;
- conventional numerical or grid-based optimization.

Useful optimization targets include plant/observer parameters, controller gains, predictive-control weights, and the free degree of freedom in unified `d1/d2` allocation.

The governing rule is simple:

> **Every advanced method must earn its complexity.**

A method that adds substantial implementation cost without useful control improvement is not required to remain in the final architecture.

### Phase 4 — Learning-Enhanced Control

Introduce neural-network methods only after the deterministic physics-based platform is working and measured data shows a clear reason for learning.

The preferred first use is a bounded residual model:

```text
physics predictor
       +
Tiny NN residual
       ↓
improved iL_hat / model prediction
```

For example:

```text
iL_hat = iL_hat_physics + ΔiL_NN
```

Other possible research paths are:

- NN-assisted state estimation;
- learned compensation of nonlinear model residuals;
- low-rate adaptive parameter scheduling;
- distillation of an expensive MPC policy into a Tiny-NN representation when the original controller is too costly for the STM32F334.

Learning never owns the safety boundary. NN outputs remain bounded and pass through deterministic constraints, and the physics-based path remains available as a fallback.

---

## Hardware Platform

The target board is the CBB024D / CBB02405D V1.2 four-switch synchronous bidirectional buck-boost converter.

![Four-switch bidirectional buck-boost topology](docs/images/four-switch-bidirectional-buck-boost-topology.svg)

### Physical switch mapping

| Function | Device / MCU pin |
| --- | --- |
| Left high-side | Q1 / PA8 `PWM1H` |
| Left low-side | Q4 / PA9 `PWM1L` |
| Right high-side | Q2 / PA10 `PWM2H` |
| Right low-side | Q3 / PA11 `PWM2L` |

### Existing measurements

| Signal | MCU pin |
| --- | --- |
| `Vin` | PA0 |
| `Iin` | PA1 |
| `Vout` | PA2 |
| `Iout` | PA3 |
| `VADJ` | PA4 |

### Nominal hardware characteristics

| Parameter | Value |
| --- | --- |
| MCU | STM32F334C8T6 |
| Switching / control rate | 200 kHz |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Suggested maximum power | 200 W |
| Main inductor | 22 µH nominal |
| Current shunts | 1 mΩ |
| Main MOSFETs | BSC070N10NS3G |
| Gate drivers | Si8233BD-D-IS |
| Host UART | USART1, PB6 / PB7 |

The hardware already measures both terminal currents, but there is no dedicated ADC channel for the main inductor current. That constraint is intentionally retained and becomes part of the state-estimation problem.

---

## Control Conventions

Physical identities do not change when the direction of energy flow changes:

```text
Port A = physical left / schematic VIN side
Port B = physical right / schematic VOUT side
```

Project conventions:

```text
Iin  > 0 : current enters the converter from Port A
Iout > 0 : current leaves the converter into Port B
iL   > 0 : inductor current flows left → right

d1 = Q1 left high-side logical duty
d2 = Q3 right low-side logical duty
```

Forward A → B operation therefore has positive `Iin`, `Iout`, and normally positive `iL`. Reverse B → A operation is represented by signed quantities rather than by swapping ADC channels, timer ownership, or physical port names.

See [`control-conventions.md`](docs/design/control-conventions.md) for the complete canonical definition.

---

## Firmware Architecture

The firmware uses deterministic bare-metal C. libopencm3 provides the low-level peripheral layer, while CMSIS-DSP / CMSIS-NN may be used where they provide useful numerical or inference primitives.

```text
Application / Power Manager
        ↓
Control / Estimation
        ↓
Unified Modulation
        ↓
Platform
        ↓
libopencm3
        ↓
STM32F334
```

The hard real-time path remains local to the MCU. Host software is supervisory only.

```text
Host CLI / Web UI
        ↓
COBS + CRC16 protocol
        ↓
USB-UART / USART1
        ↓
Power Manager / Telemetry
```

Host commands request state changes; they do not directly command MOSFETs or bypass protection.

---

## Research Philosophy

This repository follows four rules.

### 1. Do not re-prove the vendor baseline

The vendor implementation is accepted as evidence that the physical board can perform basic Buck, Boost, Mixed, forward/reverse operation, 200 kHz switching, and conventional PI/PID-based regulation.

Those capabilities are references, not project milestones.

### 2. Preserve the physics model

Advanced optimization or learning methods augment the deterministic converter model; they do not erase it.

### 3. Keep safety deterministic

Power-stage qualification, HRTIM safe-off behavior, duty constraints, minimum-pulse requirements, fault authority, and controlled shutdown remain outside GA/NN authority.

### 4. Do not force an algorithm into the project

GA, MPC, or NN is included only when data shows a problem worth solving and the method provides a useful benefit for its added complexity.

---

## Repository Layout

```text
docs/
  design/       architecture and design specifications
  images/       project-owned diagrams

firmware/
  app/          application entry points and host service
  control/      controllers and estimators
  platform/     STM32F334 peripheral implementation
  power/        measurement and power-domain abstractions
  protocol/     MCU-independent COBS/CRC protocol
  safety/       protection and supervisory implementation

test/
  power/        measurement/scaling tests
  protocol/     host-protocol tests
  control_loop/ controller tests
  protection/   state/protection tests

tools/
  analysis/     offline analysis / identification
  host_cli.py   host-side UART client
```

Future optimization and learning tools should remain host-side unless there is a clear embedded-runtime reason to deploy them.

---

## Documentation

[`docs/design/README.md`](docs/design/README.md) defines the design-document source-of-truth hierarchy.

Key documents:

- [`hardware-specification.md`](docs/design/hardware-specification.md) — physical board facts;
- [`control-conventions.md`](docs/design/control-conventions.md) — physical ports, signs, power, and duty definitions;
- [`system-architecture.md`](docs/design/system-architecture.md) — system ownership and layering;
- [`sensing-and-scaling.md`](docs/design/sensing-and-scaling.md) — measurement conversion and calibration;
- [`current-observability-and-estimation.md`](docs/design/current-observability-and-estimation.md) — `iL_hat` strategy;
- [`modulation-and-operating-regions.md`](docs/design/modulation-and-operating-regions.md) — `vL* → d1/d2` realization;
- [`protection-and-state-machine.md`](docs/design/protection-and-state-machine.md) — Power Manager and safety boundary;
- [`host-interface-and-uart-protocol.md`](docs/design/host-interface-and-uart-protocol.md) — host wire protocol;
- [`development-roadmap.md`](docs/design/development-roadmap.md) — detailed development planning.

Current task status belongs in GitHub Issues and commits, not in stable architecture documents.

---

## Safety

This converter operates with significant voltage, current, switching energy, and stored energy.

Before energized closed-loop work, the implementation must provide at least:

```text
explicit OFF state
qualified enable path
safe HRTIM inactive state
bounded duty / minimum-pulse constraints
fault authority over PWM
controlled shutdown
```

The Si8233 gate-driver `DISABLE` input is not MCU-controlled on this hardware, so safe shutdown depends on the STM32F334/HRTIM output-control architecture and available hardware fault paths.

Do not rely on online SWD debugging while the power stage is energized. Halting the MCU can leave PWM-related hardware in an unsafe condition. High-side gate and switching-node measurements require appropriate differential or isolated instrumentation.

---

## Vendor Reference Material

Vendor schematics, firmware, reports, examples, and manuals are treated as engineering reference evidence. They are used to recover physical mapping, timing, scaling, and known-good implementation details when needed.

The project does not attempt to reproduce the vendor tutorial sequence and does not redistribute third-party source archives as part of its own research output.
