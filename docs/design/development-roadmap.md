# Development Roadmap

## Purpose

This document defines the stable technical development path and research scope for the project. Current progress, active gate, and task status belong in GitHub Issues.

> **Validate the implementation delta, not the vendor-proven baseline.**

The project uses the proven CBB024D hardware as a plant and reference system. It does not repeat the vendor learning sequence unless a specific implementation question requires reference evidence.

## Project Goal

Build a modern experimental digital-power control platform for a four-switch bidirectional buck-boost converter using STM32F334.

The final platform should provide:

- deterministic and safe embedded control;
- PWM-synchronized acquisition;
- signed bidirectional voltage/current semantics;
- model-based reconstruction of main-inductor current without adding a sensor;
- unified Buck/Mixed/Boost control allocation;
- one firmware architecture for both power-flow directions;
- cascaded and advanced controllers on a common interface;
- reproducible host-side experiment control, capture, and benchmarking.

## Scope Boundary

### Vendor-proven baseline

The following are treated as established reference capability, not independent project milestones:

- four-switch synchronous power-stage operation;
- Buck, Boost, and Mixed operation;
- 200 kHz HRTIM switching capability;
- existing `Vin`, `Iin`, `Vout`, `Iout`, and `VADJ` sensing circuits;
- vendor PI / PID / Type-III examples;
- vendor CV / CC / current-limit examples;
- vendor soft-start and supervisory-protection examples;
- OLED, LED, key, SWD, and UART interfaces;
- forward and reverse converter operation demonstrated by vendor examples.

Vendor sources remain useful for pin mapping, timing, scaling, known-good peripheral configuration, and implementation recovery when uncertainty remains.

### Non-goals

The project will not spend milestones on:

- re-proving basic Buck / Boost / Mixed operation;
- reproducing vendor open-loop examples;
- reimplementing vendor PI / PID / Type-III merely for learning;
- revalidating the published 12–48 V / 200 W board capability as a project goal;
- adding a dedicated inductor-current sensor or new ADC channel solely for `iL`;
- making host/browser timing part of the real-time loop.

## Canonical Technical Backbone

All notation follows `control-conventions.md`:

```text
PWM-synchronized sensing
        ↓
calibrated Vin / Iin / Vout / Iout
        ↓
inductor-current estimator iL_hat
        ↓
voltage / energy controller
        ↓
iL_ref
        ↓
current controller
        ↓
vL*
        ↓
unified control allocation / modulation
        ↓
d1 / d2
        ↓
HRTIM
```

Canonical averaged model:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

The control law may therefore request a desired average inductor voltage:

```text
d1 Vin - (1 - d2) Vout = vL*
```

The modulation layer owns the realization of that request under region, pulse-width, dead-time, bootstrap, and protection constraints.

## Cross-phase safety prerequisite

Before any energized closed-loop experiment, the minimum Power Manager and protection boundary must already exist. At minimum:

```text
explicit OFF state
qualified enable request
safe HRTIM inactive state
bounded duty / minimum-pulse constraints
fault authority over PWM
controlled shutdown path
```

Advanced control work must never be used as the mechanism that makes the hardware safe.

---

## Phase 1 — Platform Foundation

### Objective

Establish a deterministic firmware platform that can configure required peripherals while keeping the power stage inactive until explicitly authorized.

### Work

- deterministic startup order;
- safe GPIO initialization;
- gate-driver inputs held inactive by default;
- UART transport and versioned COBS/CRC16 protocol;
- board-I/O abstraction;
- HRTIM configuration with outputs forced inactive;
- explicit HRTIM ownership handoff from GPIO safe-low state;
- Power Manager skeleton with OFF/qualification authority;
- minimum fault/PWM suppression path required before energized work;
- real-board host communication.

### Exit condition

The firmware initializes the STM32F334 and required peripherals, communicates with a host, and keeps the converter demonstrably inactive until an explicit qualified startup path is exercised.

---

## Phase 2 — Measurement and Estimation

### Objective

Create a measurement and state-estimation system accurate and deterministic enough for current-loop and model-based control.

### Work

- ADC1 DMA acquisition for `Vin` / `Iin` / `Vout` / `Iout`;
- ADC2 acquisition for `VADJ` where useful;
- HRTIM-synchronized ADC trigger timing;
- documented sample phase, conversion order, and latency;
- signed current representation from the sensing layer upward;
- affine gain/offset calibration;
- zero-current calibration;
- measurement bandwidth/noise characterization only where control-relevant;
- inductor-current observability analysis;
- model predictor for `iL_hat`;
- residual correction using measured `Vin` / `Vout`, with `Iin` / `Iout` used as measured model inputs;
- duty-conditioned algebraic/pseudo-measurement fusion only when numerically well conditioned;
- confidence/validity output;
- later Luenberger/Kalman-family comparison if justified by data.

### Fixed estimator constraint

```text
Vin, Iin, Vout, Iout, d1, d2
        ↓
      estimator
        ↓
      iL_hat
```

No additional final-architecture `iL` sensor is assumed.

### Exit condition

Calibrated signed measurements are reproducible, acquisition timing is fixed, and `iL_hat` has bounded error/delay and explicit confidence behavior over the intended envelope.

---

## Phase 3 — Unified Control Core

### Objective

Create a layered control architecture that does not require separate Buck, Mixed, and Boost control laws.

### Work

- outer voltage controller;
- optional outer energy/power controller;
- `iL_ref` generation;
- inner current controller using `iL_hat`;
- controller output expressed as `vL*`;
- unified `vL* -> d1/d2` control allocation;
- saturation and anti-windup;
- duty, minimum-pulse, and dead-time constraints;
- bootstrap-refresh constraints;
- operating-region scheduler;
- bumpless Buck/Mixed/Boost transitions;
- DCM/light-load/zero-crossing policy.

### Baseline controller

The independent baseline is cascaded voltage/current PI. It exists as the common reference architecture for later comparisons, not as a reimplementation of vendor mode-specific PID examples.

### Exit condition

One control stack regulates across Buck, Mixed, and Boost regions through one controller abstraction and one modulation layer.

---

## Phase 4 — Unified Bidirectional Operation

### Objective

Support both energy-flow directions with one physical mapping, one sign convention, and one firmware image.

### Work

- fixed Port A / Port B definitions;
- fixed signed `Iin`, `Iout`, and `iL` conventions;
- signed power-flow semantics;
- direction-aware references without ADC/timer remapping;
- reverse-current qualification;
- controlled zero-current crossing;
- forward/reverse transition management;
- regenerative-energy handling;
- pre-biased and hot-plug startup cases;
- controlled shutdown with energy flowing in either direction.

### Exit condition

A single firmware image regulates both directions while preserving the same sensing, estimation, protection, control, modulation, and host abstractions.

---

## Phase 5 — Advanced Control

### Objective

Compare advanced control methods on the common platform without changing the underlying measurement or hardware interfaces.

### Controller set

1. Cascaded PI baseline
2. LQI
3. Deadbeat Predictive Current Control
4. Super-Twisting Sliding Mode Control
5. Constrained / reduced-complexity MPC

Continuous-control-set or otherwise fixed-frequency MPC is preferred initially because it fits the existing HRTIM architecture better than a variable-frequency finite-control-set implementation.

Representative deadbeat relation:

```text
vL* = L/Ts * (iL_ref[k+1] - iL_hat[k])
```

All controllers must respect the same Power Manager, protection, estimator-quality, modulation, and timing boundaries.

### Exit condition

Multiple controllers can be selected and benchmarked under repeatable operating conditions without redefining the plant interface.

---

## Phase 6 — Experiment Platform

### Objective

Turn the converter into a practical browser-controlled digital-power laboratory while keeping all real-time authority local to the MCU.

### Host path

```text
Browser Web App
        ↓
Device API
        ↓
Protocol Codec
        ↓
Web Serial
        ↓
USB-UART
        ↓
STM32F334
```

### Work

- Web Serial connection management;
- device/capability discovery;
- state/fault display;
- voltage/current reference configuration;
- output enable/disable requests through Power Manager;
- programmable ramps and experiment sequences;
- atomic parameter management;
- telemetry;
- MCU-timestamped waveform capture;
- CSV / JSON export;
- controller selection;
- experiment metadata and reproducibility;
- benchmark automation and comparison.

The browser never participates in switching-cycle timing.

### Exit condition

The converter can be configured, exercised, captured, and benchmarked from the host without compromising deterministic local control or protection.

## Power Manager and Protection Boundary

Canonical states and transition policy are defined by `protection-and-state-machine.md`. Host commands are requests only.

Protection remains authoritative:

```text
hardware-immediate / HRTIM suppression
        ↓
modulation hard limits
        ↓
Power Manager state/qualification limits
        ↓
controller constraints
```

## Controller Benchmark Matrix

| Metric | PI | LQI | Deadbeat | ST-SMC | MPC |
| --- | --- | --- | --- | --- | --- |
| Step response | ✓ | ✓ | ✓ | ✓ | ✓ |
| Overshoot / undershoot | ✓ | ✓ | ✓ | ✓ | ✓ |
| Settling time | ✓ | ✓ | ✓ | ✓ | ✓ |
| Current ripple | ✓ | ✓ | ✓ | ✓ | ✓ |
| Disturbance rejection | ✓ | ✓ | ✓ | ✓ | ✓ |
| Buck/Mixed/Boost transition | ✓ | ✓ | ✓ | ✓ | ✓ |
| Forward/reverse transition | ✓ | ✓ | ✓ | ✓ | ✓ |
| Model sensitivity | ✓ | ✓ | ✓ | ✓ | ✓ |
| CPU cycles / WCET | ✓ | ✓ | ✓ | ✓ | ✓ |
| PWM deadline margin | ✓ | ✓ | ✓ | ✓ | ✓ |

Efficiency, thermal behavior, estimator error, and noise sensitivity are added when the experiment setup can measure them consistently.

## Shortest technical path

```text
real-board UART
        ↓
HRTIM-safe foundation + minimum protection
        ↓
PWM-synchronized ADC + DMA
        ↓
calibrated signed measurements
        ↓
iL estimator
        ↓
cascaded voltage/current PI
        ↓
unified vL* -> d1/d2 modulation
        ↓
unified bidirectional operation
        ↓
LQI / Deadbeat / ST-SMC / MPC
        ↓
Web experiment and benchmark platform
```

Development effort should stay on this path unless a prerequisite defect blocks progress.
