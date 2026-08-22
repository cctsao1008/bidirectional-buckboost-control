# Development Roadmap

## Purpose

This document defines the technical development path and research scope for the project.

The project is not intended to reproduce the vendor learning sequence or re-prove functions that are already demonstrated by the original CBB024D hardware and reference firmware. The development effort focuses on the implementation delta required to turn the board into a modern experimental digital-power control platform.

> **Validate the implementation delta, not the vendor-proven baseline.**

Progress tracking, current phase, and task status belong in GitHub Issues. This document is intentionally kept as a stable roadmap rather than a status report.

## Project Goal

Build a modern experimental digital-power control platform for a four-switch bidirectional buck-boost converter using STM32F334.

The final platform should support:

- safe deterministic embedded control;
- PWM-synchronized measurement acquisition;
- bidirectional voltage/current measurement semantics;
- model-based inductor-current estimation without adding a new current sensor;
- cascaded and advanced control laws;
- unified Buck / Mixed / Boost modulation;
- forward and reverse power flow using one firmware architecture;
- host-side experiment control, telemetry, waveform capture, and benchmarking.

## Scope Boundary

### Vendor-Proven Baseline

The following capabilities are treated as established hardware/reference-firmware baseline and are not independent development milestones:

- four-switch synchronous Buck-Boost power-stage operation;
- Buck, Boost, and Mixed operating regions;
- 200 kHz HRTIM switching capability;
- existing Vin, Iin, Vout, Iout, and VADJ sensing circuits;
- basic PI / PID / Type-III reference control examples;
- constant-voltage and constant-current examples;
- soft-start and basic supervisory protection examples;
- OLED, LED, key, SWD, and UART board interfaces;
- forward and reverse converter operation demonstrated by vendor reference examples.

Vendor sources remain useful as executable reference material for pin mapping, timing, ADC scaling, known-good peripheral configuration, and recovery when implementation details are uncertain.

### Non-Goals

The project will not spend development time on:

- re-proving basic Buck / Boost / Mixed operation;
- reproducing vendor open-loop examples;
- reimplementing vendor PI / PID / Type-III controllers merely for learning;
- revalidating vendor-proven 12-48 V / 200 W hardware capability as a project milestone;
- adding an external inductor-current sensor or a new ADC channel solely to measure `iL`;
- making browser timing part of the real-time control loop.

## Technical Backbone

The intended control path is:

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
D1 / D2
        ↓
HRTIM
```

The averaged four-switch model used as the common control abstraction is:

```text
Cin dVin/dt  = Iin - D1 iL
L   diL/dt   = D1 Vin - (1 - D2) Vout
Cout dVout/dt = (1 - D2) iL - Iout
```

The control core should therefore operate around the desired average inductor voltage:

```text
D1 Vin - (1 - D2) Vout = vL*
```

This separates the control law from the operating-region-specific modulation constraints.

---

## Phase 1 - Platform Foundation

### Objective

Establish a safe, deterministic firmware platform that can operate the board peripherals without unintentionally enabling the power stage.

### Work

- deterministic startup ordering;
- safe GPIO initialization;
- power-stage outputs forced inactive by default;
- UART transport and versioned host protocol;
- COBS framing and CRC16 integrity checks;
- board I/O abstraction;
- HRTIM configuration with outputs held inactive;
- explicit, qualified PWM enable path;
- real-board communication and peripheral bring-up.

### Exit Condition

The project firmware can initialize the STM32F334, communicate with the host, configure the required peripherals, and keep the converter power stage demonstrably inactive until explicitly authorized.

---

## Phase 2 - Measurement and Estimation

### Objective

Create a measurement system accurate and deterministic enough for current-loop and model-based control.

### Work

- ADC1 DMA acquisition for Vin / Iin / Vout / Iout;
- ADC2 acquisition for VADJ where required;
- HRTIM-synchronized ADC trigger timing;
- documented sampling phase within the PWM period;
- signed current representation from the sensing layer upward;
- affine gain/offset calibration per channel;
- zero-current calibration;
- measurement latency characterization;
- analog/digital sensing bandwidth characterization where control-relevant;
- inductor-current observability analysis;
- model predictor for `iL_hat`;
- measurement-based correction using Iin / Iout;
- confidence-weighted fusion or observer implementation;
- later comparison with Luenberger / Kalman-family estimators if useful.

### Estimator Constraint

No additional inductor-current sensor is assumed.

The estimator must use only the existing hardware measurements plus PWM/control state:

```text
Vin, Iin, Vout, Iout, D1, D2
```

A baseline model predictor is:

```text
iL_hat[k+1] = iL_hat[k]
              + Ts/L * (D1 Vin - (1 - D2) Vout)
```

Port-current relations may be used as correction information, not blindly as exact instantaneous `iL` measurements.

### Exit Condition

Calibrated signed measurements are reproducible, sampling timing is fixed, and `iL_hat` is credible enough to be used by a closed-loop current controller.

---

## Phase 3 - Unified Control Core

### Objective

Replace mode-specific vendor-style control logic with a modern layered control architecture.

### Work

- outer voltage controller;
- optional outer energy/power controller for bidirectional operation;
- generation of `iL_ref`;
- inner current controller using `iL_hat`;
- controller output expressed as desired average inductor voltage `vL*`;
- unified control allocation from `vL*` to `D1` / `D2`;
- duty, minimum-pulse, and deadtime constraints;
- bootstrap-refresh constraints;
- operating-region management;
- bumpless Buck / Mixed / Boost transitions;
- anti-windup and saturation handling;
- DCM / light-load / zero-crossing policy.

### Baseline Controller

The first modern baseline is cascaded voltage/current PI control.

It exists to provide a robust reference architecture for later advanced-controller comparisons, not to duplicate the vendor's mode-specific PID examples.

### Exit Condition

One control architecture regulates the converter across Buck, Mixed, and Boost regions without switching between separate mode-specific control laws.

---

## Phase 4 - Unified Bidirectional Operation

### Objective

Support both power-flow directions with one physical mapping, one sign convention, and one firmware architecture.

### Work

- fixed Port A / Port B physical definitions;
- fixed positive inductor-current direction;
- signed Iin / Iout conventions;
- signed power-flow convention;
- forward and reverse references using the same controller stack;
- reverse-current qualification;
- controlled zero-current crossing;
- forward/reverse transition management;
- regenerative-energy handling;
- pre-biased output and hot-plug behavior;
- safe shutdown while energy is flowing in either direction.

The architecture should avoid separate "forward firmware" and "reverse firmware" implementations.

### Exit Condition

A single firmware image can command and regulate both directions of energy flow while preserving the same sensing, state-estimation, control, protection, and host abstractions.

---

## Phase 5 - Advanced Control

### Objective

Use the common sensing, estimation, and modulation foundation to compare advanced control methods fairly.

### Controller Set

1. Cascaded PI baseline
2. LQI
3. Deadbeat Predictive Current Control
4. Super-Twisting Sliding Mode Control
5. Constrained / reduced-complexity MPC

Continuous-control-set MPC is preferred initially because fixed-frequency switching is compatible with the existing HRTIM architecture and the STM32F334 computational budget.

A representative deadbeat relation is:

```text
vL* = L/Ts * (iL_ref[k+1] - iL_hat[k])
```

All advanced controllers must use the same measurement, estimator, modulation, protection, and experiment infrastructure unless a comparison explicitly studies one of those layers.

### Exit Condition

The platform can select and benchmark multiple controllers under repeatable operating conditions without changing the underlying hardware interface or measurement conventions.

---

## Phase 6 - Experiment Platform

### Objective

Turn the converter into a practical browser-controlled digital-power laboratory.

### Host Architecture

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
- device identification and capability negotiation;
- converter state and fault display;
- voltage/current reference configuration;
- output enable/disable requests through the Power Manager;
- programmable ramps and experiment sequences;
- parameter management;
- real-time telemetry;
- MCU-timestamped local waveform capture;
- CSV / JSON export;
- controller selection;
- experiment metadata and reproducibility;
- benchmark automation and result comparison.

The browser remains supervisory. Real-time control, switching, protection, and time-critical capture remain local to the STM32F334.

### Exit Condition

The converter can be configured, exercised, captured, and benchmarked from a browser without compromising local deterministic control or protection.

---

## Power Manager and Protection Boundary

Control algorithms do not own converter enable authority.

The intended path is:

```text
Host / Local Request
        ↓
Power Manager
        ↓
Qualification
        ↓
Pre-start checks
        ↓
Soft Start
        ↓
Regulation
        ↓
Controlled Shutdown
```

Host commands such as `OUTPUT_ENABLE` are requests, never direct PWM commands.

Protection is split into two classes:

```text
Fast protection
  comparator / HRTIM / cycle-level output suppression

Supervisory protection
  voltage / current / state / timeout / recovery policy
```

The final architecture should keep protection authoritative over all controller and host requests.

## Controller Benchmark Matrix

Controllers should be compared under matched hardware, sensing, estimator, modulation, and operating conditions.

| Metric | PI | LQI | Deadbeat | ST-SMC | MPC |
| --- | --- | --- | --- | --- | --- |
| Step response | ✓ | ✓ | ✓ | ✓ | ✓ |
| Overshoot | ✓ | ✓ | ✓ | ✓ | ✓ |
| Settling time | ✓ | ✓ | ✓ | ✓ | ✓ |
| Current ripple | ✓ | ✓ | ✓ | ✓ | ✓ |
| Disturbance rejection | ✓ | ✓ | ✓ | ✓ | ✓ |
| Buck/Mixed/Boost transition | ✓ | ✓ | ✓ | ✓ | ✓ |
| Forward/reverse transition | ✓ | ✓ | ✓ | ✓ | ✓ |
| Model sensitivity | ✓ | ✓ | ✓ | ✓ | ✓ |
| CPU cycles / WCET | ✓ | ✓ | ✓ | ✓ | ✓ |
| PWM deadline margin | ✓ | ✓ | ✓ | ✓ | ✓ |

Additional metrics such as efficiency, thermal behavior, estimator error, and noise sensitivity may be added when the experiment setup can measure them consistently.

## Development Priority

The shortest technical path to the research core is:

```text
real-board UART
        ↓
PWM-synchronized ADC + DMA
        ↓
calibrated signed measurements
        ↓
iL estimator
        ↓
cascaded voltage/current PI
        ↓
unified D1/D2 modulation
        ↓
unified bidirectional operation
        ↓
LQI / Deadbeat / ST-SMC / MPC
        ↓
Web experiment and benchmark platform
```

Development effort should stay concentrated on this path unless a prerequisite defect blocks progress.
