# Power Stage

## Scope

This document defines the independently reconstructed power-stage model used by the project. It summarizes public engineering facts needed for control design without redistributing vendor documentation.

## Topology

The converter is a four-switch non-isolated synchronous bidirectional buck-boost stage composed of two half bridges connected through a single inductor.

```text
Port A ─ Half-Bridge A ─ L ─ Half-Bridge B ─ Port B
```

Switch assignment:

| Device | Role |
| --- | --- |
| Q1 | Half-Bridge A high-side MOSFET |
| Q2 | Half-Bridge A low-side MOSFET |
| Q4 | Half-Bridge B high-side MOSFET |
| Q3 | Half-Bridge B low-side MOSFET |

Nominal main power-stage parameters:

| Parameter | Value |
| --- | --- |
| Input range | 12–48 VDC |
| Output range | 5–48 VDC |
| Rated operating point | 24 V / 5 A |
| Maximum power | 200 W |
| Switching frequency | 200 kHz |
| Main inductor | 22 µH nominal, ±20 % |
| Inductor DCR | 20.5 mΩ typ |
| Bulk capacitance | 2 × 220 µF per port |
| Current shunt | 1 mΩ |
| MOSFET | BSC070N10NS3G |

## Forward Buck Operation

When the output voltage is substantially below the input voltage, Half-Bridge A is the actively modulated buck leg.

The ideal CCM relationship is:

```text
Vout / Vin ≈ D
```

During the high-side interval, the inductor voltage is approximately:

```text
VL = Vin - Vout
```

During the freewheel interval:

```text
VL = -Vout
```

The opposite half bridge is held in a state compatible with synchronous conduction and bootstrap requirements rather than being treated as an ideal static wire.

## Forward Boost Operation

When the output voltage is substantially above the input voltage, Half-Bridge B is the actively modulated boost leg.

The ideal CCM relationship is:

```text
Vout / Vin ≈ 1 / (1 - D)
```

During inductor charging:

```text
VL = Vin
```

During energy transfer to the output:

```text
VL = Vin - Vout
```

## Mixed Buck-Boost Operation

When the two port voltages are close, both half bridges participate in switching.

Using buck-side duty `D1` and boost-side duty `D2`, the ideal conversion ratio is:

```text
Vout / Vin = D1 / (1 - D2)
```

The known reference implementation uses approximately:

```text
D1 = 0.8
```

and varies `D2` to regulate the output in the mixed region.

This project treats that strategy as a baseline, not as a requirement for later controllers.

## Operating-Region Baseline

The reference forward mode scheduler uses:

| Voltage relationship | Operating mode |
| --- | --- |
| `Vout < 0.8 × Vin` | Buck |
| `0.8 × Vin ≤ Vout ≤ 1.2 × Vin` | Mixed buck-boost |
| `Vout > 1.2 × Vin` | Boost |

Future scheduling logic may introduce hysteresis, bumpless transfer, gain scheduling, or alternative modulation policies. Those changes must be justified by measured behavior.

## Bidirectional Operation

The power stage is electrically symmetric enough to support power flow in either direction. The two physical ports therefore should not be hard-coded conceptually as permanently fixed source and load roles.

A useful control abstraction is:

```text
source_port
sink_port
power_flow_direction
voltage_ratio
operating_region
```

rather than separate unrelated forward and reverse implementations.

## Main Inductor

Nominal values currently used by the project:

```text
L_nom = 22 µH
L_tol = ±20 %
DCR_typ = 20.5 mΩ
```

The inductor is not modeled as ideal. Control and robustness work should account for:

- inductance tolerance;
- DCR;
- temperature rise;
- saturation-related inductance reduction;
- current ripple and peak current.

## Port Capacitance

Each port uses two 220 µF bulk capacitors together with smaller ceramic capacitors for higher-frequency decoupling.

For control modeling, the effective capacitance and ESR should be measured or bounded rather than inferred from nominal capacitance alone.

## Main MOSFETs

The initial hardware uses BSC070N10NS3G MOSFETs.

Selected nominal device data used for engineering estimates include:

```text
VDS_max      = 100 V
RDS(on)_typ  = 6.3 mΩ
Qg_typ       = 42 nC
```

Datasheet switching times are test-condition dependent and are not assumed to equal the actual board switching times. Gate resistance, driver impedance, layout, parasitics, current, and bus voltage all affect real transitions.

## Switching Loss and Conduction Loss

A first-order loss model can separate:

```text
P_total ≈ P_conduction + P_switching + P_gate + P_inductor + P_misc
```

with conduction loss estimated from RMS current and temperature-dependent `RDS(on)`, while switching loss must ultimately be correlated with measured voltage/current overlap.

## Dead Time

Dead time is a system-level quantity, not only a timer register value.

Effective non-overlap depends on:

- MCU PWM timing;
- gate-driver dead-time insertion and propagation mismatch;
- gate resistance;
- MOSFET charge behavior;
- parasitic capacitance and diode recovery;
- operating voltage and current.

The design target is therefore **measured effective dead time at the switching devices**, not merely a programmed digital delay.

## Model Hierarchy

The power stage will be represented at several levels:

1. switching model;
2. averaged large-signal model;
3. operating-point small-signal model;
4. state-space model;
5. parameter-uncertainty model.

Each abstraction is used only where its assumptions remain valid.

## Validation Targets

Power-stage reconstruction should be validated against real measurements including:

- gate-drive timing;
- switching-node waveforms;
- inductor-current slopes and ripple;
- output-voltage ripple;
- buck / mixed / boost transitions;
- line and load transients;
- power-flow reversal where safely supported.
