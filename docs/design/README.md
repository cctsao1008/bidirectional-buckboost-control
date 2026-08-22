# Design Documentation Index

This directory contains the stable design specifications for the project. Progress, current gate, and task status belong in GitHub Issues; design documents should describe durable interfaces, conventions, constraints, and acceptance logic.

## Source-of-truth hierarchy

When two project documents overlap, use the following ownership rules instead of duplicating or locally redefining the same concept.

| Topic | Canonical document |
| --- | --- |
| Project scope, phases, non-goals | `development-roadmap.md` |
| System decomposition and ownership | `system-architecture.md` |
| Physical board facts and pin mapping | `hardware-specification.md` |
| Port, current, power, inductor-current, and duty conventions | `control-conventions.md` |
| Firmware timing/layering policy | `firmware-architecture.md` |
| Safe GPIO startup and HRTIM ownership handoff | `gpio-initialization.md` |
| Gate timing, dead time, bootstrap, PWM safety | `gate-drive-and-timing.md` |
| ADC scaling, calibration, and sampling semantics | `sensing-and-scaling.md` |
| Inductor-current observability and estimator design | `current-observability-and-estimation.md` |
| Control allocation and Buck/Mixed/Boost modulation | `modulation-and-operating-regions.md` |
| Power Manager states, protection ownership, and recovery | `protection-and-state-machine.md` |
| Host framing, commands, and wire-format enums | `host-interface-and-uart-protocol.md` |
| Model hierarchy, parameter provenance, and validation | `modeling-strategy.md` |
| Power-stage control model | `power-stage.md` |

## Canonical notation

Project notation is defined by `control-conventions.md`. In particular:

```text
Port A = left physical port  = schematic VIN side
Port B = right physical port = schematic VOUT side

iL > 0 : Port A -> Port B

d1     : average on-time fraction of Q1, left high-side
d2     : average on-time fraction of Q3, right low-side

L diL/dt = d1 Vin - (1 - d2) Vout
```

Older vendor material may use different switch numbering or uppercase `D1` / `D2`. Project documents must use the V1.2 schematic mapping and the notation above.

## Hardware source hierarchy

For physical implementation facts:

1. CBB024D V1.2 schematic/net mapping.
2. Physical board measurement when the measurement method is controlled and recorded.
3. Vendor hardware design report for design intent and derivation.
4. Vendor software design report and examples for reference implementation behavior.
5. Vendor user/product material for operating guidance and published specifications.

A conceptual vendor diagram must not override the V1.2 schematic.

## Development principle

> **Validate the implementation delta, not the vendor-proven baseline.**

Vendor material is reference evidence. The project does not spend milestones re-proving basic Buck/Boost/Mixed operation, vendor PI/PID examples, or the already demonstrated power-stage capability. Hardware tests are performed when they are required to validate new firmware, sensing, estimation, modulation, protection, or controller behavior.

## Keeping documents consistent

When a canonical definition changes, update the owning document first and make dependent documents reference it rather than introducing a second definition. Avoid embedding current progress in design documents; use Issues and commits for progress history.
