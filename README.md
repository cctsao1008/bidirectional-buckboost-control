# Bidirectional Buck-Boost Control

Digital power control and research platform for a four-switch non-isolated bidirectional buck-boost converter, covering classical and modern control methods.

## Why This Project

The initial hardware platform is an existing converter board with a known-good vendor implementation. The hardware already works, so the interesting problem is not basic bring-up and the goal is not to reproduce the vendor firmware.

This project instead uses the converter as a real physical plant for studying how digital power control should be modeled, implemented, measured, and compared.

The project focuses on:

- reconstructing the physical plant and control interfaces;
- establishing reproducible reference measurements;
- developing an independent control implementation;
- validating analytical and simulation models against real hardware;
- implementing classical and modern control methods on the same plant;
- comparing controllers under identical operating conditions.

The long-term objective is to turn the converter into a reusable **digital power control research platform** where control ideas are tested against real switching hardware rather than evaluated only in simulation.

## Design

The project is organized around several design principles.

### Physical plant first

Controller design starts from the actual converter, including switching topology, component parameters, sensing dynamics, gate-driver behavior, operating regions, and measured waveforms.

The plant is not treated as an idealized transfer function detached from the hardware.

### Independent control stack

Vendor firmware is used only as a known-good reference. New firmware, models, control laws, test infrastructure, and measurements are developed independently.

### Model ↔ hardware feedback loop

Simulation and analysis must be checked against real measurements.

```text
Physical Plant
      ↓
Measurement
      ↓
Model Identification / Validation
      ↓
Controller Design
      ↓
Implementation
      ↓
Hardware Test
      ↓
Model Update
      ↺
```

A model is useful only to the extent that it predicts the behavior of the actual converter closely enough to support control design.

### Control algorithms are interchangeable, experiments are not

The control architecture separates sensing, estimation, control law, modulation, and hardware-specific PWM implementation:

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

Different controllers should run against the same plant abstraction and be evaluated using the same experimental protocol.

### Measurement before claims

A controller is not considered successful merely because the converter operates.

Its behavior must be **explainable, measurable, reproducible, and comparable against a defined baseline**.

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

## Status

Early-stage research and architecture definition.

Current focus: **reference-system and physical-plant characterization**.

## License

To be determined.
