# Bidirectional Buck-Boost Control

Digital power control and research platform for a four-switch non-isolated bidirectional buck-boost converter, covering classical and modern control methods.

## Why This Project

The initial hardware platform is an existing converter board with a known-good vendor implementation. The hardware already works, so the interesting problem is not basic bring-up and the goal is not to reproduce the vendor firmware.

This project instead uses the converter as a real physical plant for studying how digital power control should be modeled, implemented, measured, and compared. Vendor firmware serves only as a known-good reference; new firmware, models, control laws, test infrastructure, and measurements are developed independently.

The project focuses on:

- reconstructing the physical plant and control interfaces;
- establishing reproducible reference measurements;
- developing an independent control implementation;
- validating analytical and simulation models against real hardware;
- implementing classical and modern control methods on the same plant;
- comparing controllers under identical operating conditions.

The long-term objective is to turn the converter into a reusable **digital power control research platform** where control ideas are tested against real switching hardware rather than evaluated only in simulation.

## Design

### Physical plant first

Controller design starts from the actual converter, including switching topology, component parameters, sensing dynamics, gate-driver behavior, operating regions, and measured waveforms. The plant is not treated as an idealized transfer function detached from the hardware, and simulation results are only trusted once they have been checked against measurements:

```
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

```
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

```
hardware/       pwm, adc, protection, timing
plant/          measurements, operating_regions, scaling
control/        see Control Research
supervisor/     startup, mode_selection, fault_handling
```

Different controllers run against the same plant abstraction and are evaluated using the same experimental protocol.

### Measurement before claims

A controller is not considered successful merely because the converter operates. Its behavior must be **explainable, measurable, reproducible, and comparable against a defined baseline**.

## System

The target plant is a four-switch synchronous bidirectional buck-boost converter composed of two synchronous half bridges connected through a single inductor.

![Four-switch bidirectional buck-boost topology](docs/images/four-switch-bidirectional-buck-boost-topology.svg)

The same power stage supports buck, boost, and mixed buck-boost operation in either power-flow direction.

The reference implementation divides the forward operating range into three regions:

| Region | Mode |
| --- | --- |
| `Vout < 0.8 × Vin` | Buck |
| `0.8 × Vin ≤ Vout ≤ 1.2 × Vin` | Mixed buck-boost |
| `Vout > 1.2 × Vin` | Boost |

In the reference mixed-mode strategy, the buck-side duty ratio is fixed near `D1 = 0.8` while the boost-side duty ratio `D2` is varied. The ideal conversion relationship is:

```
Vout / Vin = D1 / (1 - D2)
```

This behavior is retained as a reference baseline, but later controllers are not required to use the same modulation strategy.

## Hardware Reference

The initial physical platform has the following nominal characteristics:

| Parameter | Value |
| --- | --- |
| Topology | Four-switch non-isolated bidirectional buck-boost |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Maximum power | 200 W |
| Switching frequency | 200 kHz |
| Main inductor | 22 µH, ±20 %, DCR 20.5 mΩ typ |
| Bulk capacitance | 2 × 220 µF per power port |
| Current shunt | 1 mΩ |
| Main MOSFET | BSC070N10NS3G, 100 V, 6.3 mΩ typ, Qg 42 nC |
| Gate driver | Si8233BD-D-IS |
| Signal-conditioning op amp | GS8552 |
| Initial control MCU | STM32F334 |

The MCU is an implementation target, not the architectural identity of the project. These values will evolve into a machine-readable plant parameter database shared by simulation, analysis tools, and firmware.

## Measurement Model

### Voltage sensing

The voltage-sensing path has an approximate gain of:

```
Kv = 3.3 kΩ / 68 kΩ ≈ 0.049
```

which maps an ADC full-scale input to approximately 68 V at the power port.

### Bidirectional current sensing

The current measurement uses the 1 mΩ shunt and a differential amplifier with approximately `Ki = 0.15 V/A`. A 1.65 V offset allows bidirectional measurement using a unipolar ADC:

```
Vadc = 1.65 + 0.15 × I
I    = (Vadc - 1.65) / 0.15
```

The analog signal-conditioning and RC filtering stages are part of the effective measurement plant and will be characterized rather than treated as ideal sensors.

## Reference Baseline

The original development platform already operates with vendor firmware, which is used only as a reference system. Existing reference material also includes open-loop and compensated simulations for buck, boost, and mixed buck-boost operation.

Reference characterization includes:

- complementary PWM timing and effective dead time;
- buck, boost, and mixed-mode switching behavior;
- inductor-current ripple and output-voltage ripple;
- load-step response and soft-start behavior;
- bidirectional current polarity;
- short-circuit shutdown and restart behavior.

Vendor firmware, schematics, simulation files, datasheets, and documentation are **not redistributed** by this repository.

## Control Research

A central objective is to compare multiple control paradigms on the same physical plant.

**Classical control** — PI / PID, cascaded current and voltage loops, Type-III digital compensation, feedforward control.

**State-space control** — state-space modeling, controllability / observability analysis, full-state feedback, pole placement, state observers.

**Optimal and estimation-based control** — LQR, Luenberger observer, Kalman state estimation, LQG.

**Nonlinear and advanced control** — gain scheduling, sliding-mode control, MPC, robustness and uncertainty analysis.

**Control methods are included only when they can be evaluated against the same physical plant and experimental protocol.**

## Controller Benchmark

Controllers are compared using common engineering metrics:

| Metric | Description |
| --- | --- |
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

## Safety

This repository controls a switching power converter capable of significant voltage, current, and stored energy. Incorrect PWM polarity, timing, dead time, mode transition, or protection behavior can destroy power devices and connected equipment.

Particular attention must be given to:

- shoot-through prevention and effective dead-time validation;
- current limiting and overvoltage protection;
- safe startup and shutdown;
- reverse energy flow;
- hardware fault shutdown;
- oscilloscope grounding and differential measurement.

Standard earth-referenced oscilloscope probe grounds must not be connected directly to floating high-side switching nodes. Differential or properly isolated measurement equipment is required where appropriate.

## Status

Early-stage research and architecture definition. Current focus: **reference-system and physical-plant characterization**.

## License

To be determined.
