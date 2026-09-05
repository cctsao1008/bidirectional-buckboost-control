# Gate Drive and Timing

## Purpose

This document defines the timing boundary between STM32F334 HRTIM outputs, Si8233 gate drivers, and the four-switch power stage.

## Physical mapping

| Signal | MOSFET |
| --- | --- |
| `PWM1H` / PA8 | Q1, left high-side |
| `PWM1L` / PA9 | Q4, left low-side |
| `PWM2H` / PA10 | Q2, right high-side |
| `PWM2L` / PA11 | Q3, right low-side |

The two gate drivers are Si8233BD-D-IS devices with 10 Ω external gate resistors. Driver `DISABLE` is tied inactive and is not under MCU control on V1.2.

## Effective timing model

```text
commanded HRTIM dead time
        ↓
driver propagation / internal timing
        ↓
gate charge and discharge
        ↓
effective gate non-overlap
        ↓
power-stage commutation interval
```

A timer register value is not equivalent to measured gate non-overlap.

## Ownership

### Platform / HRTIM

- PWM period and update synchronization;
- complementary-output polarity;
- commanded dead time;
- atomic compare updates;
- deterministic inactive state;
- minimum/maximum pulse constraints;
- fault-forced output suppression;
- ADC trigger placement.

### Gate driver

- isolated level translation;
- propagation delay and channel mismatch;
- local non-overlap behavior;
- high-side bootstrap drive.

### Modulation

- legal `d1/d2` and `e1/e2` range;
- minimum pulse/off-time policy;
- bootstrap-refresh constraints;
- bounded actuation movement;
- saturation metadata.

## Bootstrap constraint

High-side drive uses bootstrap circuitry. A static high-side command is therefore subject to minimum refresh/off-time requirements. Modulation treats bootstrap refresh as a hard realizability constraint rather than a controller assumption.

## Safe HRTIM handoff

```text
PA8..PA11 GPIO safe-low
        ↓
configure HRTIM timebase / polarity / dead time / faults
        ↓
force all HRTIM outputs inactive
        ↓
switch PA8..PA11 to HRTIM alternate function
        ↓
confirm inactive outputs
        ↓
Power Manager qualification
        ↓
explicit PWM enable
```

Peripheral initialization alone never authorizes switching.

## PWM update rules

The implementation provides:

- synchronized atomic duty updates;
- deterministic period-boundary behavior;
- complementary non-overlap;
- defined enable and disable sequences;
- bounded minimum and maximum pulse widths;
- fault authority over controller output;
- deterministic saturation behavior.

Controllers never write raw HRTIM compare registers.

## ADC timing relationship

At 200 kHz:

```text
Tsw = 5 us
```

ADC trigger timing is part of the switching-cycle definition. Sample phase accounts for switching-edge settling, ripple phase, conversion order/latency, analog-filter delay, and the actuation deadline.

## Timing acceptance conditions

The power-stage timing boundary is valid only when:

1. PA8–PA11 remain inactive through reset and HRTIM ownership transfer.
2. HRTIM polarity matches the physical gate-driver inputs.
3. Complementary pairs do not overlap through duty transitions.
4. Effective gate non-overlap is compatible with the power stage.
5. Minimum/maximum pulse and bootstrap constraints are enforced by modulation.
6. Fault forcing produces the defined inactive gate state.
7. ADC trigger phase is deterministic relative to PWM.

## Measurement safety

High-side gate-source and switching-node measurements require differential or otherwise isolated instrumentation. Earth-referenced probe grounds are not connected to floating high-side nodes.

Online SWD halt/debug is not used while the energized power stage depends on continuous firmware PWM execution.