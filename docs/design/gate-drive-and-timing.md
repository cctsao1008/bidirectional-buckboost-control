# Gate Drive and Timing

## Purpose

This document defines the timing boundary between STM32F334 HRTIM outputs, Si8233 gate drivers, and the four-switch power stage. The board is already known to operate with vendor firmware; project measurements here are limited to validating the **new implementation delta** and establishing constraints required by new modulation/control code.

## Physical Mapping

| Signal | MOSFET |
| --- | --- |
| `PWM1H` / PA8 | Q1, left high-side |
| `PWM1L` / PA9 | Q4, left low-side |
| `PWM2H` / PA10 | Q2, right high-side |
| `PWM2L` / PA11 | Q3, right low-side |

The gate drivers are Si8233BD-D-IS devices with external 10 Ω gate resistors. Driver `DISABLE` is not under MCU control on V1.2.

## Effective Timing Is a System Quantity

The project distinguishes:

```text
commanded HRTIM dead time
        ↓
driver propagation / internal non-overlap
        ↓
gate charge/discharge dynamics
        ↓
effective gate non-overlap
        ↓
power-stage commutation interval
```

A timer register value is therefore not accepted as proof of effective dead time.

## Timing Ownership

### HRTIM / platform layer

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
- local overlap/dead-time behavior;
- high-side drive and bootstrap behavior.

### Modulation layer

- legal `d1` / `d2` range;
- minimum off-time/pulse policy;
- bootstrap-refresh policy;
- region transitions;
- bounded actuation changes.

## Bootstrap Constraint

The high-side drivers use bootstrap circuitry. A logically static high-side request is not automatically valid indefinitely. Modulation must maintain enough low-side/off-time activity to keep the driver supplied under the actual operating condition.

Bootstrap limits are treated as explicit hardware constraints. They should be derived from authoritative device information and confirmed only as needed for the project’s chosen modulation envelope.

## Safe HRTIM Handoff

The mandatory sequence is:

```text
PA8..PA11 GPIO safe-low
        ↓
configure HRTIM base/timers
        ↓
configure polarity / dead time / fault behavior
        ↓
force all HRTIM outputs inactive
        ↓
switch PA8..PA11 to HRTIM alternate function
        ↓
verify inactive outputs
        ↓
Power Manager qualification
        ↓
explicit PWM enable
```

Peripheral initialization alone never authorizes switching.

## PWM Update Rules

The implementation must provide:

- synchronized/atomic duty updates;
- deterministic period-boundary behavior;
- no unintended complementary overlap;
- defined enable and disable sequences;
- bounded minimum and maximum pulse width;
- fault authority that overrides controller output;
- known behavior if a new command saturates or becomes unrealizable.

Controllers never write raw HRTIM compare registers.

## ADC Timing Relationship

At 200 kHz:

```text
Tsw = 5 us
```

ADC timing is part of the switching-cycle design. Sample placement must account for switching-edge settling, deterministic ripple phase, conversion order/latency, analog-filter delay, and the controller’s actuation deadline.

## Required Implementation-Delta Verification

Before applying significant bus power with project firmware, verify only what the project changes or depends on:

1. PA8–PA11 remain inactive through reset and HRTIM ownership transfer.
2. HRTIM output polarity matches the physical gate-driver inputs.
3. Complementary pairs never overlap under commanded duty transitions.
4. Effective gate non-overlap is compatible with the power stage.
5. Minimum/maximum pulse and bootstrap constraints are enforced by modulation.
6. Fault/disable forcing produces the intended inactive gate state.
7. ADC trigger phase is deterministic relative to PWM.

This is **not** a standalone re-characterization campaign for vendor-proven Buck/Boost/Mixed waveforms.

## Measurement Safety

High-side gate-source and switching-node measurements require differential or otherwise properly isolated instrumentation. Standard earth-referenced probe grounds must not be connected to floating high-side nodes.

The vendor documentation also warns against online debugging while converter power is applied; pausing firmware can make PWM behavior unsafe.

## Design Rules

1. Effective timing is validated at the gate/power-stage boundary when required, not inferred from code alone.
2. HRTIM configuration is separate from HRTIM enable authority.
3. Duty and pulse constraints belong below controller code.
4. ADC timing and PWM timing are designed together.
5. Timing evidence records operating voltage/current, probe method, and firmware revision.
6. Do not repeat broad vendor baseline tests unless a new implementation depends on them.
