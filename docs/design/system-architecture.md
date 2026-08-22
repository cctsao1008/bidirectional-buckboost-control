# System Architecture

## Purpose

This document defines the high-level architecture and ownership boundaries of the bidirectional buck-boost control platform. Detailed signs, hardware facts, state-machine enums, modulation rules, and wire formats are owned by their dedicated design documents listed in `docs/design/README.md`.

The architecture follows two principles:

> **The physical converter is the source of truth.**

> **Validate the implementation delta, not the vendor-proven baseline.**

## System Boundary

The controlled system includes more than the ideal four-switch stage. The effective plant and implementation boundary include:

- two synchronous half bridges and the main inductor;
- port capacitances and source/load interaction;
- MOSFET/gate-driver timing and loss;
- voltage/current sensing and analog filtering;
- ADC sample timing and conversion latency;
- HRTIM actuation timing and dead time;
- operating-region allocation;
- Power Manager startup/shutdown behavior;
- protection and fault forcing.

## Canonical Physical Mapping

The V1.2 hardware mapping is:

```text
Port A / left                         Port B / right

      Q1 high                              Q2 high
         |                                    |
         +----------- L1 = 22 uH -------------+
         |                                    |
      Q4 low                               Q3 low
         |                                    |
        GND----------------------------------GND
```

All project-owned firmware and documentation use this mapping. `hardware-specification.md` owns the full board facts.

## Canonical Control Path

```text
Physical Power Stage
        ↓
ADC / Signal Conditioning
        ↓
Calibration / Scaling
        ↓
Vin / Iin / Vout / Iout
        ↓
State Estimation
        ↓
iL_hat + estimator health
        ↓
Outer Voltage / Energy Controller
        ↓
iL_ref
        ↓
Inner Current Controller
        ↓
vL*
        ↓
Unified Control Allocation / Modulation
        ↓
d1 / d2
        ↓
HRTIM / Gate Drive
        ↓
Physical Power Stage
```

The key averaged actuation relation is:

```text
L diL/dt = d1 Vin - (1 - d2) Vout
```

The controller is therefore organized around desired average inductor voltage `vL*`; Buck/Mixed/Boost realization belongs to modulation.

## Measurement and Estimation Boundary

The board measures `Vin`, `Iin`, `Vout`, and `Iout`, but not `iL` directly. The target architecture does not add an inductor-current sensor.

```text
PWM-synchronized ADC/DMA
        ↓
calibrated signed measurements
        ↓
model predictor + residual correction
        ↓
iL_hat / confidence
```

`Iin` and `Iout` are terminal currents, not unconditional instantaneous `iL` measurements.

## Controller Boundary

Controllers consume logical physical quantities and produce a logical actuation objective. They do not:

- write HRTIM registers;
- decide GPIO alternate-function ownership;
- bypass minimum-pulse/dead-time/bootstrap limits;
- enable the power stage;
- override protection.

The independent baseline is cascaded voltage/current PI. Advanced comparison targets are LQI, Deadbeat Predictive Current Control, Super-Twisting SMC, and constrained/reduced MPC.

## Modulation Boundary

The modulation layer owns:

- conversion of `vL*` into realizable `d1` / `d2`;
- Buck/Mixed/Boost region policy;
- duty and pulse-width limits;
- bootstrap refresh;
- bounded/bumpless region transitions;
- hardware-realizable complementary switching requests;
- saturation feedback needed by anti-windup.

The vendor 0.8/1.2 region boundaries and mixed-mode strategy are reference evidence, not permanent architectural constraints.

## Power Manager Boundary

The Power Manager is the authority for whether switching is allowed. Canonical externally visible states are defined in `protection-and-state-machine.md`:

```text
OFF
 ↓
QUALIFY
 ↓
SOFT_START
 ↓
REGULATION
 ↓
SHUTDOWN
 ↓
OFF
```

Fault paths may enter `FAULT` and, when policy allows, `RETRY_WAIT` before re-qualification.

Boot/reset is an internal initialization condition, not a reason to expose an unsafe partially initialized converter state.

## Protection Boundary

Protection is layered and independent of the selected controller:

```text
hardware-immediate / HRTIM output suppression
        ↓
modulation hard limits
        ↓
Power Manager qualification and state policy
        ↓
controller constraints
```

A controller experiment is never allowed to become the sole mechanism preventing hardware damage.

## Bidirectional Operation

Port identities never swap:

```text
Port A = left / schematic VIN side
Port B = right / schematic VOUT side
```

Direction changes are represented by signed current/power, references, estimator state, Power Manager policy, and modulation. The same ADC channels, physical switch mapping, and control abstraction remain valid in both directions.

The target is one firmware image for A→B and B→A energy transfer.

## Host and Instrumentation Boundary

Host software is supervisory:

```text
CLI or Browser
      ↓
Device / Protocol API
      ↓
COBS + CRC16
      ↓
USB-UART / USART1
      ↓
Power Manager / Telemetry
```

The Windows-side CLI is appropriate for first physical UART validation. Web Serial is the later experiment-platform client. Neither is part of the 200 kHz real-time loop.

High-rate transient capture is MCU-timestamped and buffered locally; browser timing is never used as measurement timing.

## Model-to-Hardware Loop

Model work exists only to support the new estimator/control/modulation questions:

```text
select operating condition
        ↓
model / predict
        ↓
simulate or calculate
        ↓
measure only required implementation delta
        ↓
compare quantitative metrics
        ↓
update model / assumptions
```

The project does not perform broad characterization merely to duplicate vendor evidence.

## Design Intent, Implementation, Evidence

Every significant subsystem should keep these separate:

```text
design intent    what must happen
implementation   how firmware/hardware does it
evidence         measurements/tests proving the delta
```

This separation is the basis for repeatable controller comparison and safe iteration.
