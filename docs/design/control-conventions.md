# Control and Power-Flow Conventions

## Purpose

This document is the canonical definition of the project’s physical naming, sign conventions, and duty variables. Models, firmware, telemetry, estimators, controllers, tests, and host tools must follow these definitions.

The CBB024D V1.2 schematic is the physical source of truth. Vendor examples 12 and 13 are useful reference implementations, but their forward/reverse channel-remapping strategy is not adopted by this project.

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

Older conceptual vendor diagrams that imply different half-bridge pairings must not override this mapping.

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

Both are normally non-negative board-terminal voltages.

## Current convention

```text
Iin  > 0 : current enters the converter from Port A
Iout > 0 : current leaves the converter into Port B
```

Therefore:

```text
Forward A -> B : Iin > 0, Iout > 0
Reverse B -> A : Iin < 0, Iout < 0
```

Negative current values are valid measurements and must not be clipped by the common sensing layer.

The physical polarity of each populated shunt/amplifier path must be verified once against this convention during measurement bring-up. After that, calibration may correct sign/gain/offset but must not reinterpret channels according to direction.

## Power convention

```text
Pin  = Vin  * Iin
Pout = Vout * Iout
```

With the current convention above:

```text
Forward power flow : Pin > 0, Pout > 0
Reverse power flow : Pin < 0, Pout < 0
```

Energy balance is written as:

```text
dE/dt = Pin - Pout - Ploss
```

Example reverse steady state:

```text
Port B supplies 100 W -> Pout = -100 W
Port A receives 95 W  -> Pin  = -95 W
Loss                  -> 5 W

Pin - Pout - Ploss = -95 - (-100) - 5 = 0
```

## Inductor-current convention

```text
               iL > 0
Port A  -------------------->  Port B
```

Thus:

```text
iL > 0 : left-to-right inductor current
iL < 0 : right-to-left inductor current
```

The board has no direct ADC channel for `iL`. Controllers that require it consume the reconstructed state `iL_hat`.

## Canonical duty convention

Project documents use lower-case `d1` and `d2`:

```text
d1 = average on-time fraction of Q1, left high-side switch
d2 = average on-time fraction of Q3, right low-side switch
```

The corresponding ideal averaged inductor equation is:

```text
L diL/dt = d1 Vin - (1 - d2) Vout
```

At ideal steady state:

```text
Vout / Vin = d1 / (1 - d2)
```

Some vendor documents use uppercase `D1` / `D2`; when those quantities refer to the same logical duties, they map to project `d1` / `d2`. Project-owned documentation should use the lower-case notation consistently.

The modulation layer translates logical `d1` / `d2` into physical HRTIM compare values, complementary outputs, dead time, minimum pulse behavior, and bootstrap-compatible gate commands. Controllers must not depend on timer polarity or compare-register encoding.

## Direction and operating region are separate concepts

Power-flow direction and Buck/Mixed/Boost operating region are not synonyms.

```text
power-flow direction : A->B or B->A
operating region     : Buck / Mixed / Boost
```

A single firmware image must preserve the same physical ADC and switch mapping in both directions. Direction is represented by references, measured signs, Power Manager policy, estimator state, and modulation behavior.

## Host implications

Current wire fields retain board names:

```text
vin_mV   : unsigned
vout_mV  : unsigned
iin_mA   : signed
iout_mA  : signed
```

Future `iL` telemetry must be signed. Direction-neutral UI may label the terminals Port A and Port B while retaining the existing wire names for compatibility.

## Mandatory rules

1. ADC acquisition never swaps physical channels because power direction changes.
2. Calibration never clips valid negative current.
3. `iL_hat` follows the fixed left-to-right sign convention.
4. Controllers operate on physical/logical quantities, not raw HRTIM register semantics.
5. Modulation owns `d1` / `d2` realization, timing, dead time, minimum pulse, and bootstrap constraints.
6. Power Manager owns whether a requested energy-flow direction is permitted.
7. Protection evaluates signed physical quantities explicitly.
8. New documentation must reference this file instead of redefining current, power, or duty signs independently.

## Vendor reference evidence

Vendor example 12 interprets ADC1 as `Vin`, `Iin`, `Vout`, `Iout` for forward operation. Vendor example 13 changes application interpretation and reverses current-offset handling for reverse operation, and it changes which timer receives the Buck/Boost update. That proves useful hardware capability, but the project intentionally replaces application-specific remapping with the fixed conventions above.
