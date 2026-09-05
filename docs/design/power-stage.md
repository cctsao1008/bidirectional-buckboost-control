# Power Stage

## Purpose

This document defines the control model of the CBB024D V1.2 four-switch power stage. Detailed board facts belong to `hardware-specification.md`.

## Physical topology

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

| Bridge | High-side | Low-side | PWM signals |
| --- | --- | --- | --- |
| Left / Port A | Q1 | Q4 | `PWM1H`, `PWM1L` |
| Right / Port B | Q2 | Q3 | `PWM2H`, `PWM2L` |

## Nominal parameters

| Parameter | Value |
| --- | --- |
| Port-A voltage range | 12–48 VDC published input range |
| Port-B voltage range | 5–48 VDC published output range |
| Rated operating point | 24 V / 5 A |
| Suggested maximum power | 200 W |
| Switching frequency | 200 kHz |
| Main inductor | 22 µH nominal, ±20% |
| Inductor DCR | approximately 20.5 mΩ typical reference |
| Nominal port capacitance | approximately 460 µF including local 10 µF capacitors |
| Terminal-current shunts | 1 mΩ |
| MOSFET | BSC070N10NS3G |
| Gate driver | Si8233BD-D-IS |

Nominal values are model parameters, not calibrated truth.

## Canonical averaged model

```text
d1 = Q1 left high-side average duty
d2 = Q3 right low-side average duty
iL > 0 from Port A to Port B
```

Ideal CCM equations:

```text
Cin  dVin/dt  = Iin - d1 iL
L    diL/dt   = d1 Vin - (1 - d2) Vout
Cout dVout/dt = (1 - d2) iL - Iout
```

With effective duties:

```text
e1 = d1
e2 = 1 - d2
```

```text
L diL/dt = Vin e1 - Vout e2
```

The central actuation coordinate is:

```text
vL = Vin e1 - Vout e2
```

At ideal steady state:

```text
Vout / Vin = d1 / (1 - d2)
           = e1 / e2
```

## Operating-point descriptions

### Buck-like

When Port B voltage is below Port A voltage, an efficient allocation lies near a right-leg pass-through boundary and primarily varies the left bridge.

Ideal limiting relation:

```text
Vout / Vin ≈ d1
```

### Boost-like

When Port B voltage is above Port A voltage, an efficient allocation lies near a left-leg pass-through boundary and primarily varies the right bridge.

Ideal limiting relation:

```text
Vout / Vin ≈ 1 / (1 - d2)
```

### Mixed-like

When both bridges participate materially:

```text
Vout / Vin = d1 / (1 - d2)
```

The control architecture does not require these descriptions to become separate controller states.

## Bidirectional semantics

Port identities remain fixed:

```text
Port A = left physical terminal
Port B = right physical terminal
```

Reverse energy flow is represented by signed current/power and controller references. The physical bridge mapping does not change.

## Non-ideal model terms

The control model includes a non-ideal term only when its parameter and effect are explicitly represented. Relevant terms include:

- inductor DCR and inductance variation;
- capacitor ESR/effective capacitance;
- MOSFET conduction loss;
- dead-time voltage error;
- switching loss;
- source/load impedance;
- sensing and actuation delay.

`modeling-strategy.md` owns the model-layer and parameter-validity rules.

## Timing boundary

Effective commutation depends on HRTIM timing, Si8233 propagation, gate resistance, MOSFET charge, dead time, and operating point. `gate-drive-and-timing.md` owns these constraints.

## Sensing boundary

The board directly measures port voltage/current only:

```text
Vin / Iin / Vout / Iout
```

Main-inductor current is reconstructed as `iL_hat`; external `iL` instrumentation is validation-only.