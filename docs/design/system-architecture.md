# System Architecture

## Purpose

This document defines the high-level architecture and ownership boundaries of the bidirectional buck-boost control system.

The existing CBB024D V1.2 power hardware is the physical plant. The project-owned system is the sensing, estimation, control, modulation, protection, firmware, and host architecture around that plant.

## System boundary

The controlled system includes:

- two synchronous half bridges and the main inductor;
- port capacitance and source/load interaction;
- MOSFET/gate-driver timing;
- voltage/current sensing and analog filtering;
- ADC sample timing and conversion latency;
- HRTIM actuation timing and dead time;
- state estimation;
- controller state and references;
- constrained duty allocation;
- Power Manager startup/shutdown behavior;
- protection and fault forcing;
- supervisory telemetry/protocol interfaces.

## Physical mapping

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

`hardware-specification.md` owns board facts; `control-conventions.md` owns signs and duties.

## Canonical control path

```text
Physical Power Stage
        ↓
PWM-synchronized ADC / signal conditioning
        ↓
Calibration / scaling
        ↓
Vin / Iin / Vout / Iout
        ↓
State estimator
        ↓
iL_hat + estimator validity
        ↓
Outer voltage / energy control
        ↓
iL_ref
        ↓
Inner current control
        ↓
vL*
        ↓
Continuous constrained allocator
        ↓
e1 / e2
        ↓
d1 / d2
        ↓
HRTIM / gate drive
        ↓
Physical Power Stage
```

The common actuation equation is:

```text
L diL/dt = d1 Vin - (1 - d2) Vout
         = Vin e1 - Vout e2
```

## Measurement and estimation boundary

The board directly measures:

```text
Vin
Iin
Vout
Iout
```

Main-inductor current is reconstructed:

```text
realized actuation + calibrated measurements
                    ↓
            model predictor
                    ↓
       measurement correction
                    ↓
          iL_hat + validity
```

`Iin` and `Iout` are terminal currents, not instantaneous `iL` substitutes.

## Controller boundary

Controllers consume calibrated logical quantities and produce physical actuation objectives. They do not:

- write HRTIM registers;
- own GPIO alternate-function handoff;
- bypass duty/dead-time/bootstrap constraints;
- authorize switching;
- override protection.

The controller-to-modulation interface is `vL*`.

## Modulation boundary

The modulation layer owns:

- feasibility clamp for `vL*`;
- continuous `e1/e2` allocation;
- conversion to `d1/d2`;
- duty/minimum-pulse/off-time limits;
- bootstrap refresh;
- complementary-output legality;
- saturation metadata;
- realized actuation `vL_realized`.

Buck-like, Mixed-like, and Boost-like labels describe the resulting operating point; they are not controller states.

## Power Manager boundary

The Power Manager owns switching authority:

```text
OFF
 ↓
QUALIFY
 ↓
SOFT_START
 ↓
REGULATION
 ↓
SHUTDOWN
 ↓
OFF
```

Fault paths use `FAULT` and, where policy permits, `RETRY_WAIT`.

Host commands and controller outputs are requests subordinate to this state machine.

## Protection boundary

```text
hardware-immediate / HRTIM suppression
        ↓
modulation hard limits
        ↓
Power Manager qualification/state policy
        ↓
controller constraints
```

Protection behavior is independent of controller family.

## Bidirectional operation

```text
Port A = left / schematic VIN side
Port B = right / schematic VOUT side
```

Port identity, ADC channels, switch mapping, and firmware ownership do not change with power-flow direction. Direction is represented by signed current/power, references, estimator state, and Power Manager policy.

## Host boundary

```text
Host client
      ↓
COBS + CRC16 protocol
      ↓
USART1
      ↓
Power Manager / telemetry
```

The host is supervisory. It is not a switching-cycle scheduler, measurement clock, or direct PWM endpoint.

## Architecture invariants

1. Physical mapping is fixed by the V1.2 schematic.
2. Signed physical conventions are direction-independent.
3. The controller requests `vL*`; modulation realizes constrained duties.
4. The estimator consumes realized actuation rather than unconstrained requests.
5. Power Manager owns switching authority.
6. Protection is controller-independent.
7. Host timing is outside the real-time control loop.
8. All hard real-time peripheral ownership remains local to STM32F334 firmware.