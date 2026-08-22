# Bidirectional Buck-Boost Control

A modern experimental digital-power control platform for a four-switch non-isolated bidirectional buck-boost converter using STM32F334.

The physical board already has a known-good vendor implementation. This project therefore does **not** repeat the vendor learning sequence or spend milestones re-proving basic Buck, Boost, Mixed, PI/PID, or 200 W power-stage capability. Vendor material is used as reference evidence while the project focuses on the implementation delta needed for modern sensing, estimation, unified bidirectional control, advanced controllers, and reproducible experiments.

> **Validate the implementation delta, not the vendor-proven baseline.**

## Project Goal

Turn the converter into a reusable digital-power research platform where control methods can be implemented and compared on the same switching hardware, with common sensing, protection, modulation, timing, and experiment infrastructure.

The technical backbone is:

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

For the project duty convention:

```text
L diL/dt = d1 Vin - (1 - d2) Vout
```

The controller is therefore not required to contain separate Buck, Mixed, and Boost control laws. Region-specific switching constraints belong in the modulation layer.

## Hardware Baseline

The target board is a four-switch synchronous bidirectional buck-boost converter with two half bridges and one main inductor.

![Four-switch bidirectional buck-boost topology](docs/images/four-switch-bidirectional-buck-boost-topology.svg)

Physical V1.2 mapping:

| Function | Device / pin |
| --- | --- |
| Left high-side | Q1 / PA8 `PWM1H` |
| Left low-side | Q4 / PA9 `PWM1L` |
| Right high-side | Q2 / PA10 `PWM2H` |
| Right low-side | Q3 / PA11 `PWM2L` |
| `Vin` | PA0 |
| `Iin` | PA1 |
| `Vout` | PA2 |
| `Iout` | PA3 |
| `VADJ` | PA4 |
| USART1 | PB6 TX / PB7 RX |

Nominal hardware characteristics:

| Parameter | Value |
| --- | --- |
| Input voltage | 12–48 VDC |
| Output voltage | 5–48 VDC |
| Rated output | 24 V / 5 A |
| Suggested maximum power | 200 W |
| Switching frequency | 200 kHz |
| Main inductor | 22 µH nominal |
| Port-current shunts | 1 mΩ |
| Main MOSFET | BSC070N10NS3G |
| Gate driver | Si8233BD-D-IS |
| MCU | STM32F334C8T6 |

The board measures both terminal currents but has no dedicated ADC channel for main-inductor current. The final architecture therefore reconstructs `iL` from existing measurements and converter state rather than adding a new current sensor.

## Control and Research Scope

The baseline controller for the independent architecture is cascaded voltage/current PI using reconstructed `iL_hat`. Advanced comparison targets are:

- LQI;
- Deadbeat Predictive Current Control;
- Super-Twisting Sliding Mode Control;
- constrained / reduced-complexity MPC.

All controller comparisons should use the same measurement, estimator, protection, modulation, and experiment infrastructure unless a study explicitly targets one of those layers.

Primary engineering metrics include settling time, overshoot, current ripple, disturbance rejection, Buck/Mixed/Boost transition behavior, forward/reverse transition behavior, model sensitivity, CPU cycles / WCET, and PWM deadline margin.

## Bidirectional Architecture

Port identities are fixed physically:

```text
Port A = left / schematic VIN side
Port B = right / schematic VOUT side
```

Power-flow direction changes do not swap ADC channels or reinterpret timer ownership. Signed currents, signed `iL`, and signed power represent direction while the physical mapping remains fixed.

The long-term target is one firmware image supporting both directions of energy flow through one sensing, estimation, control, protection, and host architecture.

## Firmware Architecture

The firmware is deterministic bare-metal C using libopencm3, with CMSIS-DSP used where it provides useful numerical primitives. The 200 kHz hard-real-time path remains independent of host timing.

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

Host communication is supervisory:

```text
Browser or CLI
      ↓
COBS + CRC16 protocol
      ↓
USB-UART / USART1
      ↓
Power Manager / Telemetry
```

`OUTPUT_ENABLE` is always a request to the Power Manager. Host software never directly enables PWM or commands individual MOSFET states.

## Repository Layout

```text
docs/
  design/       stable architecture and design specifications
  images/       project-owned diagrams

firmware/
  app/          application entry points and host service
  control/      controllers and estimators
  platform/     STM32F334 peripheral implementation
  power/        measurements and power-domain abstractions
  protocol/     MCU-independent COBS/CRC protocol
  safety/       protection and supervisory implementation

test/
  power/        measurement/scaling tests
  protocol/     host-protocol tests
  control_loop/ controller tests
  protection/   state/protection tests

tools/
  analysis/     offline analysis and identification
  host_cli.py   host-side UART bring-up client
```

## Documentation

Start with [`docs/design/README.md`](docs/design/README.md), which defines document ownership and source-of-truth hierarchy.

Key specifications:

- [`development-roadmap.md`](docs/design/development-roadmap.md) — scope, phases, non-goals
- [`system-architecture.md`](docs/design/system-architecture.md) — system decomposition and ownership
- [`hardware-specification.md`](docs/design/hardware-specification.md) — physical board facts
- [`control-conventions.md`](docs/design/control-conventions.md) — port/current/power/duty conventions
- [`current-observability-and-estimation.md`](docs/design/current-observability-and-estimation.md) — `iL_hat` strategy
- [`modulation-and-operating-regions.md`](docs/design/modulation-and-operating-regions.md) — `vL* -> d1/d2`
- [`protection-and-state-machine.md`](docs/design/protection-and-state-machine.md) — Power Manager and protection
- [`host-interface-and-uart-protocol.md`](docs/design/host-interface-and-uart-protocol.md) — wire protocol

Current progress is intentionally tracked in GitHub Issues and commits rather than embedded in stable design documents.

## Development Safety

The converter can switch significant voltage, current, and stored energy. New firmware must establish a demonstrably inactive power stage before energized tests. PWM polarity, complementary timing, HRTIM safe-off behavior, fault forcing, minimum pulse constraints, and bootstrap requirements are implementation prerequisites, not optional tuning details.

The vendor documentation also warns against online SWD debugging while the converter is powered because paused or tri-stated PWM can damage the power stage. High-side gate and switching-node measurements require differential or otherwise properly isolated instrumentation.

## Vendor Reference Material

Vendor firmware, schematics, simulation files, datasheets, and manuals are reference inputs and are not redistributed by this repository. Public project artifacts should remain independently reproducible and should document the engineering facts they rely on rather than copying third-party archives.
