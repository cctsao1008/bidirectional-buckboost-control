# System Architecture

## Purpose

This document defines the high-level architecture of the bidirectional buck-boost control platform. It sits between the project-level intent in `README.md` and the detailed implementation in firmware, models, tests, and results.

The architecture is organized around one principle: the physical converter is the source of truth. Models, control laws, and software abstractions must remain traceable to measurable converter behavior.

## System Boundary

The controlled system includes more than the ideal four-switch power stage. The effective plant includes:

- the synchronous four-switch bidirectional buck-boost topology;
- the main inductor and port capacitances;
- MOSFET and gate-driver behavior;
- sensing gains, offsets, and analog filtering;
- PWM timing and dead time;
- operating-region transitions;
- supervisory logic such as startup, shutdown, and fault handling.

The project does not treat the converter as a single ideal transfer function detached from these implementation details.

## Architecture Layers

```text
Physical Power Stage
        ↓
Sensing / Signal Conditioning
        ↓
Scaling / State Reconstruction
        ↓
Control Law
        ↓
Modulation / Operating-Region Logic
        ↓
PWM / Gate Drive
        ↓
Physical Power Stage
```

Supervisory control operates across this loop and is responsible for enabling, startup, mode transitions, shutdown, and fault recovery.

## Power Stage

The hardware is a four-switch non-isolated bidirectional buck-boost converter built from two synchronous half bridges connected through a single inductor.

- Left half-bridge: Q1 high-side / Q4 low-side
- Right half-bridge: Q2 high-side / Q3 low-side
- Main inductor: 22 µH nominal
- Switching frequency: 200 kHz nominal

This mapping follows the V1.2 schematic and MCU PWM routing and is the implementation source of truth.

The same power stage supports energy flow in either direction.

## Operating Regions

For forward power flow, the reference implementation uses three operating regions:

| Condition | Region |
| --- | --- |
| `Vout < 0.8 × Vin` | Buck |
| `0.8 × Vin ≤ Vout ≤ 1.2 × Vin` | Mixed buck-boost |
| `Vout > 1.2 × Vin` | Boost |

In the reference mixed-mode strategy, the buck-side duty ratio is held near `D1 = 0.8` while the boost-side duty ratio `D2` is adjusted, with the ideal relationship:

```text
Vout / Vin = D1 / (1 - D2)
```

This is treated as a reference modulation strategy rather than a permanent architectural constraint. Later control methods may use different duty-allocation policies.

## Measurement Architecture

The converter measures both voltage and current at the two power ports.

The current channels are bidirectional and are centered around a 1.65 V bias before entering the MCU ADC. The analog conditioning path and output RC filters are considered part of the effective measurement plant and must be characterized.

Measurement handling is separated into:

```text
raw ADC acquisition
        ↓
offset / gain calibration
        ↓
engineering-unit scaling
        ↓
optional filtering / estimation
        ↓
controller state inputs
```

The board does not provide a dedicated ADC channel for main-inductor current. If a controller requires `iL`, it must use a reconstructed state derived from the existing Vin/Iin/Vout/Iout measurements and converter state. No additional current sensor is assumed by the project architecture.

## Control Architecture

Control laws are intentionally separated from hardware-specific PWM and ADC code.

The control layer may contain classical, state-space, optimal, observer-based, nonlinear, or predictive controllers, but each controller must use a common plant interface and be evaluated under a common test protocol.

The architecture should allow the controller to be replaced without redefining the measurement conventions, operating-point definitions, or experimental metrics.

## Modulation and PWM

The modulation layer converts controller outputs into valid half-bridge commands.

Its responsibilities include:

- duty-ratio constraints;
- operating-region selection;
- complementary output generation;
- effective dead-time handling;
- safe transitions between buck, mixed, and boost operation;
- bootstrap-refresh and minimum-pulse constraints;
- preventing invalid simultaneous switch states.

The effective gate timing is a combined property of MCU PWM generation, gate-driver behavior, propagation delay, gate resistance, MOSFET switching behavior, and hardware dead-time mechanisms.

## Supervisory State Machine

The converter should be controlled through explicit system states rather than by enabling closed-loop PWM immediately after reset.

A suitable high-level state model is:

```text
OFF
 ↓
QUALIFY
 ↓
SOFT_START
 ↓
REGULATION
 ↓
FAULT
 ↓
RETRY / OFF
```

Exact transitions and thresholds will be defined from hardware characterization and independent implementation requirements.

## Protection Architecture

Protection is part of the control architecture, not an afterthought.

The design must account for:

- shoot-through prevention;
- overcurrent protection;
- overvoltage protection;
- input undervoltage / overvoltage qualification;
- safe shutdown;
- reverse energy flow;
- startup current and duty limiting;
- hardware-assisted fault response where available.

No software control experiment should bypass the minimum hardware-safety mechanisms required to protect the power stage.

## Host and Instrumentation Boundary

Host control is supervisory only:

```text
Web Browser
    ↓
Web Serial
    ↓
USB-to-UART
    ↓
COBS + CRC protocol
    ↓
Power Manager / Telemetry
```

The browser must not participate in switching-cycle control. Communication loss must not remove local protection or make PWM timing dependent on host timing.

## Model-to-Hardware Loop

The architecture uses continuous model validation:

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

A model is accepted only when its predictive accuracy is sufficient for the control question being studied.

## Design Rule

The project separates three concerns:

- **design intent** — what the system is supposed to do;
- **implementation** — how firmware and hardware execute that intent;
- **evidence** — measurements and tests that show whether the implementation matches the design.

GitHub stores the independently developed and reproducible project artifacts. Private vendor reference material and raw archival evidence remain outside the public repository.
