# Control and Power-Flow Conventions

## Purpose

This document is the canonical definition of physical naming, sign conventions, and duty coordinates. Models, firmware, telemetry, estimators, controllers, tests, and host tools use these definitions.

## Physical bridge mapping

```text
                         L1
              left node ---- right node
                 |              |
             Q1 high        Q2 high
                 |              |
             Q4 low         Q3 low
                 |              |
                GND------------GND
```

| MCU pin | Signal | Physical device |
| --- | --- | --- |
| PA8 | `PWM1H` | Q1, left high-side |
| PA9 | `PWM1L` | Q4, left low-side |
| PA10 | `PWM2H` | Q2, right high-side |
| PA11 | `PWM2L` | Q3, right low-side |

## Port naming

```text
Port A = left physical port  = schematic VIN side
Port B = right physical port = schematic VOUT side
```

The wire-level names `Vin`, `Iin`, `Vout`, and `Iout` are retained because they match the board signals. They do not imply permanent source/load roles.

## Voltage convention

```text
Vin  = VA+ - VA-
Vout = VB+ - VB-
```

## Current convention

```text
Iin  > 0 : current enters the converter from Port A
Iout > 0 : current leaves the converter into Port B
```

Therefore:

```text
A -> B power flow : Iin > 0, Iout > 0
B -> A power flow : Iin < 0, Iout < 0
```

Negative current is valid and is never clipped or reinterpreted by direction-dependent channel swapping.

## Power convention

```text
Pin  = Vin  * Iin
Pout = Vout * Iout

dE/dt = Pin - Pout - Ploss
```

With the current convention above:

```text
A -> B : Pin > 0, Pout > 0
B -> A : Pin < 0, Pout < 0
```

## Inductor-current convention

```text
               iL > 0
Port A  -------------------->  Port B
```

```text
iL > 0 : left-to-right inductor current
iL < 0 : right-to-left inductor current
```

The board has no direct ADC channel for `iL`. Control functions that require it use `iL_hat`.

## Duty convention

```text
d1 = average on-time fraction of Q1, left high-side
d2 = average on-time fraction of Q3, right low-side
```

The ideal CCM averaged inductor equation is:

```text
L diL/dt = d1 Vin - (1 - d2) Vout
```

At ideal steady state:

```text
Vout / Vin = d1 / (1 - d2)
```

Effective-duty coordinates are:

```text
e1 = d1
e2 = 1 - d2
```

so:

```text
L diL/dt = Vin e1 - Vout e2
```

The controller actuation coordinate is desired average inductor voltage `vL*`; modulation owns conversion to realizable `e1/e2` and `d1/d2`.

## Direction and operating region

Power-flow direction and operating region are independent concepts:

```text
power-flow direction : A -> B or B -> A
operating description: Buck-like / Mixed-like / Boost-like
```

Port identity, ADC mapping, and switch ownership never change with direction.

## Host field semantics

```text
vin_mV   : unsigned Port A voltage
iin_mA   : signed Port A current
vout_mV  : unsigned Port B voltage
iout_mA  : signed Port B current
```

Any `iL` telemetry is signed using the left-to-right convention.

## Mandatory rules

1. ADC acquisition never swaps physical channels because power direction changes.
2. Calibration preserves valid negative current.
3. `iL_hat` uses the fixed left-to-right sign convention.
4. Controllers operate on physical/logical quantities, not raw HRTIM register semantics.
5. Modulation owns duty realization, timing, dead time, minimum pulse, and bootstrap constraints.
6. Power Manager owns whether switching and requested energy flow are permitted.
7. Protection evaluates signed physical quantities explicitly.
8. Other documents reference these conventions instead of redefining them.