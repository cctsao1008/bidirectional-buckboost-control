# Bidirectional Buck-Boost Control

Digital power control and research platform for a four-switch non-isolated bidirectional buck-boost converter, covering classical and modern control methods.

## Overview

This project develops an independent digital control stack for a four-switch synchronous bidirectional buck-boost converter.

The initial hardware platform is an existing converter board with a known-good vendor implementation. The hardware is therefore treated as a validated physical plant and reference system rather than as a new hardware bring-up project.

**The goal is not to reproduce the vendor firmware.**

Instead, this repository focuses on:

- reconstructing the physical plant and control interfaces;
- establishing reproducible reference measurements;
- developing an independent firmware architecture;
- validating analytical and simulation models against real hardware;
- implementing classical and modern control methods;
- comparing controllers under identical operating conditions.

The long-term objective is to turn the converter into a reusable **digital power control research platform**.

## System

The target plant is a four-switch synchronous bidirectional buck-boost converter composed of two synchronous half bridges connected through a single inductor.

<p align="center">
  <img src="docs/images/four-switch-bidirectional-buck-boost-topology.svg"
       alt="Four-switch bidirectional buck-boost topology"
       width="850">
</p>

The same power stage supports forward and reverse energy flow. Depending on the voltage relationship between the two ports, the converter can operate in:

- Buck mode
- Boost mode
- Mixed Buck-Boost mode
- Reverse Buck
- Reverse Boost
- Bidirectional power-flow operation

The reference implementation divides the forward operating range into three regions:

- `Vout < 0.8 × Vin` → Buck mode
- `0.8 × Vin ≤ Vout ≤ 1.2 × Vin` → Mixed Buck-Boost mode
- `Vout > 1.2 × Vin` → Boost mode

In the reference mixed-mode strategy, the Buck-side duty ratio is fixed near `D1 = 0.8`, while the Boost-side duty ratio `D2` is varied. The ideal conversion relationship is:

```text
Vout / Vin = D1 / (1 - D2)
```

This behavior is retained as a reference baseline, but later controllers are not required to use the same modulation strategy.

## Hardware Reference

The initial physical platform has the following nominal characteristics:

| Parameter | Value |
|---|---:|
| Topology | Four-switch non-isolated bidirectional buck-boost |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Maximum power | 200 W |
| Switching frequency | 200 kHz |
| Main inductor | 22 µH |
| Bulk capacitance | 2 × 220 µF per power port |
| Current shunt | 1 mΩ |
| Main MOSFET | BSC070N10NS3G |
| Gate driver | Si8233BD-D-IS |
| Signal-conditioning op amp | GS8552 |
| Initial control MCU | STM32F334 |

The MCU is an implementation target, not the architectural identity of the project.

## Measurement Model

### Voltage sensing

The voltage-sensing path has an approximate gain of:

```text
Kv = 3.3 kΩ / 68 kΩ ≈ 0.049
```

which maps an ADC full-scale input to approximately 68 V at the power port.

### Bidirectional current sensing

The current measurement uses a 1 mΩ shunt and a differential amplifier with approximately:

```text
Ki = 0.15 V/A
```

A 1.65 V offset allows bidirectional current measurement using a unipolar ADC:

```text
Vadc = 1.65 + 0.15 × I
I    = (Vadc - 1.65) / 0.15
```

The analog signal-conditioning and RC filtering stages are part of the effective measurement plant and will be characterized rather than treated as ideal sensors.

## Control Architecture

The project separates the physical plant, measurement path, estimation, control law, modulation, and hardware-specific PWM implementation.

```text
Physical Plant
      ↓
Sensing / Scaling
      ↓
State Estimation
      ↓
Control Law
      ↓
Modulation
      ↓
PWM / Gate Driver
      ↓
Physical Plant
```

The intended software structure is conceptually divided into:

```text
hardware/
    pwm
    adc
    protection
    timing

plant/
    measurements
    operating_regions
    scaling

control/
    pi
    type3
    state_feedback
    lqr
    observer
    lqg
    gain_scheduling
    smc
    mpc

supervisor/
    startup
    mode_selection
    fault_handling
```

## Reference Baseline

The original development platform already operates with vendor firmware. That implementation is used only as a **reference system**.

Reference characterization includes:

- complementary PWM timing;
- effective dead time;
- Buck switching behavior;
- Boost switching behavior;
- mixed-mode switching behavior;
- inductor-current ripple;
- output-voltage ripple;
- load-step response;
- soft-start behavior;
- bidirectional current polarity;
- short-circuit shutdown and restart behavior.

Reference simulation material also exists for:

- Buck open loop
- Boost open loop
- Buck-Boost open loop
- Buck PID control
- Buck Type-III compensation
- Boost PID control
- Boost Type-III compensation

Vendor firmware, schematics, simulation files, datasheets, and documentation are **not redistributed** by this repository.

## Control Research

A central objective of this project is to compare multiple control paradigms on the same physical plant.

### Classical control

- PI / PID
- Cascaded current and voltage loops
- Type-III digital compensation
- Feedforward control

### State-space control

- State-space modeling
- Controllability / observability analysis
- Full-state feedback
- Pole placement
- State observers

### Optimal and estimation-based control

- Linear Quadratic Regulator (LQR)
- Luenberger observer
- Kalman state estimation
- Linear Quadratic Gaussian control (LQG)

### Nonlinear and advanced control

- Gain scheduling
- Sliding-mode control
- Model Predictive Control (MPC)
- Robustness and uncertainty analysis

**The intention is not to implement algorithms simply because they are available. Each controller must be evaluated against a common experimental protocol.**

## Development Gates

### Gate 0 — Reference-System and Physical-Plant Characterization

Reconstruct and validate:

- topology and switching states;
- plant parameters;
- sensing gains and offsets;
- PWM and gate-driver timing;
- operating-region behavior;
- baseline transient and protection behavior.

### Gate 1 — Independent Firmware Baseline

Reimplement the fundamental converter operation using independently developed firmware and reproduce the reference Buck, Boost, mixed-mode, and bidirectional behavior.

### Gate 2 — Unified Bidirectional Control Architecture

Build a common control and supervisory architecture for:

- forward and reverse power flow;
- Buck, mixed, and Boost regions;
- startup and shutdown;
- protection;
- mode transitions.

### Gate 3 — Classical Control Baseline

Implement and characterize:

- voltage PI/PID;
- current control;
- cascaded loops;
- Type-III compensation;
- feedforward.

### Gate 4 — Plant Modeling and Identification

Develop and validate:

- averaged models;
- small-signal models;
- state-space models;
- operating-point models;
- model-to-hardware correlation.

### Gate 5 — State Feedback

Full-state feedback and pole-placement control.

### Gate 6 — LQR

Quadratic optimal control with explicit state and control-effort weighting.

### Gate 7 — Observer-Based Control

State reconstruction using a Luenberger observer.

### Gate 8 — LQG

Kalman state estimation combined with LQR.

### Gate 9 — Gain-Scheduled Control

Controller scheduling across Buck, mixed, and Boost operating regions.

### Gate 10 — Nonlinear / Sliding-Mode Control

Evaluate nonlinear control and robustness against plant variation and disturbances.

### Gate 11 — Model Predictive Control

Constraint-aware predictive control considering limits such as inductor current, duty ratio, output voltage, and switching behavior.

### Gate 12 — Robustness and Uncertainty

Evaluate controller sensitivity to:

- inductor tolerance and saturation;
- capacitor and ESR variation;
- input-voltage range;
- load range;
- temperature;
- measurement noise;
- model mismatch.

### Gate 13 — Unified Controller Benchmark

Compare all applicable controllers under the same experimental protocol.

## Controller Benchmark

Controllers should be compared using common engineering metrics:

| Metric | Description |
|---|---|
| Rise time | Reference-step response |
| Overshoot / undershoot | Maximum voltage deviation |
| Settling time | Recovery time |
| Load-step deviation | `ΔVout` under load transients |
| Line transient | Response to `Vin` disturbance |
| Peak inductor current | Electrical stress |
| Current ripple | Inductor-current quality |
| Output ripple | Steady-state regulation |
| Control effort | Duty / switching-state activity |
| Efficiency | Conversion efficiency |
| Robustness | Performance under parameter variation |
| Computational cost | Execution time and memory usage |

The objective is to expose engineering trade-offs rather than declare a single universally superior controller.

## Physical Plant Parameters

Initial nominal values include:

```text
L           = 22 µH
L tolerance = ±20 %
DCR_typ     = 20.5 mΩ

Cbulk       = 2 × 220 µF

Rshunt      = 1 mΩ

MOSFET      = BSC070N10NS3G
VDS_max     = 100 V
RDS(on)_typ = 6.3 mΩ
Qg_typ      = 42 nC

fsw         = 200 kHz
```

These values will evolve into a machine-readable plant parameter database shared by simulation, analysis tools, and firmware.

## Safety

This repository controls a switching power converter capable of significant voltage, current, and stored energy.

Incorrect PWM polarity, timing, dead time, mode transition, or protection behavior can destroy power devices and connected equipment.

Particular attention must be given to:

- shoot-through prevention;
- effective dead-time validation;
- current limiting;
- overvoltage protection;
- safe startup and shutdown;
- reverse energy flow;
- hardware fault shutdown;
- oscilloscope grounding and differential measurement.

Standard earth-referenced oscilloscope probe grounds must not be connected directly to floating high-side switching nodes. Differential or properly isolated measurement equipment is required where appropriate.

## Repository Structure

Planned structure:

```text
bidirectional-buckboost-control/
├── README.md
├── firmware/
├── hardware/
├── control/
├── models/
│   └── parameters/
├── tools/
├── tests/
├── results/
└── docs/
    └── images/
```

## Engineering Loop

```text
Understand the physical plant
        ↓
Build the model
        ↓
Validate the model
        ↓
Design the controller
        ↓
Implement it
        ↓
Measure the real system
        ↓
Compare prediction and measurement
        ↓
Update the model
        ↺
```

**A controller is not considered successful merely because the converter operates. It must be explainable, measurable, reproducible, and comparable against a defined baseline.**

## Status

Early-stage research and architecture definition.

Current focus: **Gate 0 — Reference-System and Physical-Plant Characterization**.

## License

To be determined.
