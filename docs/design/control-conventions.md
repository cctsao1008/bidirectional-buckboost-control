# Control and Power-Flow Conventions

## Purpose

This document freezes the logical conventions used by models, firmware, telemetry, estimators, controllers, and tests. The goal is to prevent forward/reverse operation from changing the meaning of variables or silently remapping physical ADC channels.

The CBB024D V1.2 schematic is the physical source of truth. Vendor examples 12 and 13 are reference implementations, but their forward/reverse remapping strategy is not adopted by this project.

## Physical bridge mapping

The V1.2 schematic defines the physical switch arrangement as:

```text
                     L1
        left node -------- right node
            |                 |
        Q1 high           Q2 high
            |                 |
        Q4 low            Q3 low
            |                 |
           GND---------------GND
```

MCU mapping:

| MCU pin | Signal | Physical device |
| --- | --- | --- |
| PA8 | `PWM1H` | Q1, left high-side |
| PA9 | `PWM1L` | Q4, left low-side |
| PA10 | `PWM2H` | Q2, right high-side |
| PA11 | `PWM2L` | Q3, right low-side |

The conceptual Q1/Q2/Q3/Q4 labeling used in some older vendor diagrams must not override the V1.2 schematic mapping.

## Port naming

The firmware uses fixed physical port names:

```text
Port A = left physical port  = schematic VIN side
Port B = right physical port = schematic VOUT side
```

These names do not change when power reverses.

For compatibility with the existing board and host protocol:

```text
Vin  = voltage at Port A
Iin  = current associated with Port A
Vout = voltage at Port B
Iout = current associated with Port B
```

The names `Vin` and `Vout` are retained as hardware signal names. They do not imply that Port A must always source power or that Port B must always consume power.

## Voltage convention

Port voltages are defined as positive-terminal voltage relative to the corresponding negative terminal:

```text
Vin  = VA+ - VA-
Vout = VB+ - VB-
```

Under normal board operation both values are expected to be non-negative.

## Current convention

Current signs are defined to preserve intuitive forward and reverse power-flow semantics:

```text
Iin  > 0 : current enters the converter from Port A
Iout > 0 : current leaves the converter into Port B
```

Therefore:

```text
Forward A -> B operation: Iin > 0, Iout > 0
Reverse B -> A operation: Iin < 0, Iout < 0
```

Negative current values must not be clipped to zero in the common measurement layer.

This differs from the vendor forward example, which clips negative current samples to zero, and from the vendor reverse example, which swaps ADC channel interpretation and reverses current-offset subtraction. The project instead keeps one physical channel mapping and represents direction with signed values.

## Power convention

Define:

```text
Pin  = Vin  * Iin
Pout = Vout * Iout
```

With the current convention above:

```text
Forward power flow: Pin > 0 and Pout > 0
Reverse power flow: Pin < 0 and Pout < 0
```

The converter energy balance is written as:

```text
dE/dt = Pin - Pout - Ploss
```

This relation remains valid in both directions.

Example reverse steady state:

```text
Port B supplies 100 W  -> Pout = -100 W
Port A receives 95 W   -> Pin  = -95 W
Loss                   -> 5 W

Pin - Pout - Ploss = -95 - (-100) - 5 = 0
```

## Inductor-current convention

Inductor current is defined positive from the left bridge toward the right bridge:

```text
               iL > 0
Port A  -------------------->  Port B
```

Thus:

```text
iL > 0 : left-to-right inductor current
iL < 0 : right-to-left inductor current
```

The board has no direct ADC channel for `iL`; controllers that require it consume the reconstructed value `iL_hat`.

## Duty convention

Two logical duty variables are used:

```text
d1 = average on-time fraction of Q1, left high-side switch
d2 = average on-time fraction of Q3, right low-side switch
```

These definitions match the common averaged four-switch model:

```text
L * diL/dt = d1 * Vin - (1 - d2) * Vout
```

At ideal steady state:

```text
Vout / Vin = d1 / (1 - d2)
```

The modulation layer is responsible for translating logical `d1` and `d2` into the physical HRTIM compare values and complementary gate signals. Controllers must not depend on timer polarity or compare-register direction.

## Forward and reverse operation

Forward/reverse direction is a system state, not a different set of sensor names.

The vendor examples use two application-specific mappings:

```text
Example 12: forward Buck-Boost voltage-loop PID
Example 13: reverse Buck-Boost voltage-loop PID
```

The reverse example changes ADC channel interpretation and swaps Timer A/Timer B roles. That is useful evidence that the hardware is bidirectional, but it is not the architecture used here.

This project uses:

```text
fixed physical ADC mapping
fixed physical switch mapping
signed currents
signed iL
signed power
one control abstraction
```

Direction changes are therefore represented by references, measured signs, state-machine policy, and modulation behavior rather than by recompiling a different physical mapping.

## Host protocol implications

The host protocol already defines:

```text
vin_mV   : unsigned
vout_mV  : unsigned
iin_mA   : signed
iout_mA  : signed
```

Future telemetry for inductor current must also be signed.

The Web UI may label the two ports as Port A / Port B when direction-neutral presentation is important, while retaining Vin/Vout field names on the wire for compatibility.

## Rules for firmware modules

The following rules are mandatory:

1. ADC acquisition never swaps channels because of power direction.
2. Calibration never clips valid negative current values.
3. The estimator uses the fixed `iL > 0` left-to-right convention.
4. Controllers operate on logical physical quantities, not raw HRTIM compare values.
5. Modulation owns timer polarity, complementary outputs, dead time, bootstrap constraints, and compare-register encoding.
6. Power Manager owns whether forward or reverse energy transfer is permitted.
7. Protection evaluates signed quantities explicitly; it must not rely on a direction-specific reinterpretation of ADC channels.

## Source evidence from vendor firmware

Vendor example 12 uses the ADC sequence as:

```text
ADC1_RESULT[0] -> Vin
ADC1_RESULT[1] -> Iin
ADC1_RESULT[2] -> Vout
ADC1_RESULT[3] -> Iout
```

Vendor example 13 reverses the application direction by interpreting the same ADC buffer as:

```text
ADC1_RESULT[2] -> Vin
ADC1_RESULT[3] -> Iin, with reversed offset subtraction
ADC1_RESULT[0] -> Vout
ADC1_RESULT[1] -> Iout, with reversed offset subtraction
```

It also swaps which HRTIM timer receives the Buck/Boost compare update.

This project intentionally replaces that application-specific remapping with the fixed conventions in this document.
