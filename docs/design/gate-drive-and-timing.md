# Gate Drive and Timing

## Purpose

This document defines the timing boundary between the MCU PWM implementation, the isolated gate drivers, and the MOSFET power stage.

The main design rule is that **effective switch timing is a property of the complete signal path**, not of the MCU timer alone.

## Gate-Drive Chain

The reference hardware uses two Si8233BD-D-IS isolated dual gate drivers to drive the two synchronous half bridges.

Each half bridge therefore has the following control path:

```text
MCU PWM
   ↓
input conditioning
   ↓
Si8233BD-D-IS
   ↓
gate resistor
   ↓
MOSFET gate
   ↓
switch-node response
```

The MOSFETs are BSC070N10NS3G devices with 10 Ω external gate resistors in the reference hardware.

## Dead Time Is a System Quantity

The reference hardware includes dead-time / overlap-protection capability in the Si8233 driver, while the STM32F334 HRTIM can also generate complementary outputs and programmable timing.

The resulting non-overlap seen at the MOSFET gates may therefore include contributions from:

- MCU/HRTIM edge placement;
- driver dead-time configuration;
- driver propagation delay and channel mismatch;
- gate resistance and MOSFET gate charge;
- switching-node transition time;
- measurement delay and probe loading.

For this reason the project distinguishes:

```text
commanded dead time
```

from:

```text
effective gate non-overlap
```

and from:

```text
effective power-stage commutation interval
```

Only the latter two can be validated directly on the physical converter.

## Timing Ownership

The intended architecture assigns responsibilities as follows:

### MCU / HRTIM

- deterministic PWM period;
- complementary edge generation where used;
- update synchronization;
- bounded duty commands;
- deterministic output enable / disable;
- fault-response behavior;
- ADC trigger placement.

### Gate Driver

- isolated drive translation;
- local overlap protection / dead-time behavior;
- high-side drive implementation;
- gate-source charge and discharge current.

### Power Stage

- actual turn-on / turn-off transition;
- diode conduction during dead time;
- reverse-recovery and output-capacitance effects;
- switch-node slew rate;
- commutation loss.

## Bootstrap Constraint

The reference hardware uses bootstrap-based high-side drive. Therefore a nominally "static" high-side state may still require periodic low-side activity or another refresh mechanism depending on operating condition and driver implementation.

This is particularly relevant in operating regions where one half bridge is intended to remain near a fixed state.

The control architecture must therefore distinguish between:

- logical steady state;
- commanded PWM state;
- gate-driver refresh requirement.

Exact refresh limits should be established from driver requirements and hardware measurements rather than assumed.

## PWM Update Rules

PWM changes should be applied synchronously and atomically where possible. A controller should never leave a half bridge in an unintended intermediate combination because one compare value was updated before another.

Required properties include:

- synchronized duty updates;
- deterministic period boundary behavior;
- no transient complementary overlap;
- known behavior when enabling or disabling outputs;
- known behavior on a fault event;
- bounded minimum and maximum pulse width.

## ADC Timing Relationship

At 200 kHz switching frequency, the PWM period is:

```text
Tsw = 5 µs
```

ADC sampling is therefore part of PWM timing design, not an independent background task.

Sampling points should eventually be selected based on:

- switching-node settling;
- current-ripple phase;
- voltage-sense filter delay;
- region-specific switching state;
- control-loop latency.

The same sampling convention must be used when comparing controllers.

## Measurement Requirements

Initial timing characterization should capture, under current-limited and otherwise safe conditions:

- MCU PWM output where accessible;
- high-side and low-side gate-source voltage;
- switch-node voltage;
- inductor current;
- PWM period and duty;
- rising-edge and falling-edge non-overlap;
- transition variation with load and operating mode.

High-side gate and switching-node measurements require differential or otherwise properly isolated instrumentation. Standard earth-referenced probe grounds must not be connected to floating switching nodes.

## Design Rules

1. Never infer effective dead time from firmware configuration alone.
2. No controller directly controls raw gate GPIOs.
3. PWM enable / disable is a supervised state transition.
4. Duty limits and minimum pulse widths are enforced below the controller layer.
5. ADC triggering and PWM timing are designed together.
6. Timing measurements are recorded with operating conditions and probe configuration.
7. Hardware protection remains active during controller experiments.

## Open Characterization Items

The following values should remain explicitly unresolved until measured or derived from authoritative device data:

- effective driver dead time with the populated DT network;
- channel-to-channel propagation mismatch on the actual board;
- minimum practical pulse width;
- bootstrap refresh limit in each operating mode;
- optimal ADC sampling phase;
- switch-node transition time as a function of load and voltage.

These are experimental parameters, not assumptions to be hidden in firmware.