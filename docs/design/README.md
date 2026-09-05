# Design Documentation

This directory contains the current design source of truth for the project.

Design documents describe the system **as it is defined now**: interfaces, equations, mappings, constraints, state semantics, protocol definitions, and acceptance conditions. They do not contain development chronology, progress tracking, decision history, or roadmap material. Git history and GitHub Issues own that information.

## Source-of-truth ownership

| Topic | Canonical document |
| --- | --- |
| System decomposition and ownership | `system-architecture.md` |
| Physical board facts and pin mapping | `hardware-specification.md` |
| Port, current, power, inductor-current, and duty conventions | `control-conventions.md` |
| Firmware timing and layering | `firmware-architecture.md` |
| Safe GPIO startup and HRTIM ownership handoff | `gpio-initialization.md` |
| Gate timing, dead time, bootstrap, and PWM safety | `gate-drive-and-timing.md` |
| ADC scaling, calibration, and sampling semantics | `sensing-and-scaling.md` |
| Inductor-current observability and estimator architecture | `current-observability-and-estimation.md` |
| Control allocation and modulation | `modulation-and-operating-regions.md` |
| Power Manager states, protection ownership, and recovery | `protection-and-state-machine.md` |
| Host framing, commands, and wire-format enums | `host-interface-and-uart-protocol.md` |
| Model hierarchy and parameter semantics | `modeling-architecture.md` |
| Power-stage control model | `power-stage.md` |

## Canonical notation

Project notation is defined by `control-conventions.md`:

```text
Port A = left physical port  = schematic VIN side
Port B = right physical port = schematic VOUT side

iL > 0 : Port A -> Port B

d1     : Q1 left high-side average duty
d2     : Q3 right low-side average duty

e1 = d1
e2 = 1 - d2

L diL/dt = d1 Vin - (1 - d2) Vout
         = Vin e1 - Vout e2
```

The V1.2 schematic/net mapping is authoritative for physical switch and signal identity.

## Hardware source hierarchy

For physical implementation facts:

1. CBB024D V1.2 schematic/net mapping.
2. Controlled physical board measurement.
3. Vendor hardware design report for component intent and derivation.
4. Vendor software report/examples for peripheral reference behavior.
5. Vendor user/product material for published operating specifications.

A conceptual topology drawing does not override the V1.2 schematic.

## Documentation rule

Every Markdown file in this repository represents the current result.

Do not place the following in design Markdown:

```text
phase history
current progress
next-step lists
why a previous design was rejected
vendor-vs-project narrative
chronological experiment notes
temporary planning
```

When a design changes, replace the old definition with the new one. The commit diff preserves the history.