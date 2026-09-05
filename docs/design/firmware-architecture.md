# Firmware Architecture

## Purpose

This document defines the STM32F334 firmware layering, timing classes, ownership boundaries, and safety rules.

## Technology baseline

| Area | Definition |
| --- | --- |
| MCU | STM32F334C8T6 |
| Peripheral library | libopencm3 |
| Numerical library | CMSIS-DSP where useful |
| RTOS | None |
| Scheduling | interrupt-driven hard real-time + cooperative background loop |
| Host transport | USART1 PB6/PB7 |
| Framing | COBS + `0x00` delimiter |
| Integrity | CRC-16/CCITT-FALSE |
| Host client | `tools/host_cli.py` |

The switching-cycle path has no RTOS scheduler dependency.

## Timing budget

At 200 kHz:

```text
Tsw = 5 us
```

At a 64 MHz Cortex-M4 clock, one switching period contains approximately 320 core cycles. Measurement handoff, fast estimation/control, constraints, modulation, HRTIM update, and bounded instrumentation share this deadline.

## Timing classes

```text
Hard real-time
────────────────────────
PWM/HRTIM update
synchronized measurement handoff
fast estimator/current control
modulation hard limits
critical fault response

Soft real-time
────────────────────────
Power Manager
reference ramps
slow protection
estimator-health supervision

Background
────────────────────────
UART protocol
telemetry
diagnostics
parameter management
capture transfer
```

## Switching-cycle interrupt

The switching-cycle ISR is limited to bounded fixed-deadline work:

- consume the synchronized measurement set;
- update fast estimator state;
- execute the active fast controller;
- apply actuation constraints;
- execute unified modulation;
- atomically update HRTIM compare values;
- record bounded timing/capture metadata;
- apply time-critical fault state.

It does not allocate memory, block, parse UART, format strings, or depend on host availability.

## USART receive interrupt

The UART ISR transfers bytes into an SPSC ring buffer only:

```text
USART RXNE IRQ
      ↓
ring_buffer_push_isr()
      ↓
return
```

COBS decoding, CRC checking, dispatch, and response generation execute in background context.

## Main loop

```text
while (1)
{
    host_service_run();
    power_manager_service();
    slow_protection_service();
    telemetry_service();
    diagnostics_service();
}
```

Periodic background work is released by monotonic time state, not blocking delays.

## Layering and ownership

```text
Application / Power Manager
        ↓
Control / Estimation
        ↓
Unified Modulation
        ↓
Platform API
        ↓
libopencm3 / STM32F334 helpers
        ↓
STM32F334
```

### `firmware/app`

Owns application composition and background services. It has no direct PWM authority.

### `firmware/control`

Owns estimator and controller logic. Controllers consume calibrated physical quantities and produce logical actuation requests. They do not write timer registers.

### `firmware/power`

Owns hardware-independent converter data structures: calibrated measurements, references, powers, and converter state.

### `firmware/platform`

Owns clocks, GPIO, UART, ADC/DMA, HRTIM, timing counters, and direct libopencm3/register access.

### `firmware/protocol`

Owns MCU-independent COBS framing, CRC, packet format, and stream parsing.

### `firmware/safety`

Owns fault evaluation, Power Manager state, qualification, shutdown, and controller-independent protection policy.

## Canonical control path

```text
PWM-synchronized ADC/DMA
        ↓
calibration / scaling
        ↓
Vin / Iin / Vout / Iout
        ↓
iL estimator
        ↓
iL_hat
        ↓
outer voltage / energy control
        ↓
iL_ref
        ↓
inner current control
        ↓
vL*
        ↓
continuous constrained allocator
        ↓
e1 / e2
        ↓
d1 / d2
        ↓
platform HRTIM API
```

The board has no direct `iL` ADC channel.

## HRTIM ownership

```text
GPIO safe-low
   ↓
configure HRTIM timebase / polarity / dead time / faults
   ↓
force HRTIM outputs inactive
   ↓
switch PA8..PA11 to HRTIM alternate function
   ↓
confirm inactive state
   ↓
Power Manager qualification
   ↓
explicit enable
```

HRTIM configuration and switching authority are separate operations.

## Host protocol boundary

```text
USART IRQ
    ↓
RX ring buffer
    ↓
protocol stream collector
    ↓
COBS / version / length / CRC validation
    ↓
command dispatch
    ↓
Power Manager / telemetry
```

Malformed traffic has no path to raw PWM authority.

## Numerical library policy

CMSIS-DSP provides numerical primitives. Controller state, saturation, anti-windup, estimator state, instrumentation, and benchmark semantics remain project-owned.

## Parameter update policy

Control parameters use validated shadow-to-active transfer:

```text
shadow parameters
      ↓
validation
      ↓
atomic safe-point commit
      ↓
active parameters
```

A switching-cycle calculation never observes a partially updated parameter set.

## Safety rules

1. Host commands are supervisory requests only.
2. Output enable passes through Power Manager qualification.
3. Mandatory protection is independent of controller selection and host availability.
4. Gate-driver `DISABLE` is not MCU-controlled on V1.2; safe-off authority is implemented through GPIO/HRTIM behavior.
5. No dynamic allocation, blocking I/O, mutex, logging, or formatting is permitted in the 200 kHz path.
6. Every controller used at the switching rate has bounded execution time within the PWM deadline.
7. Controllers never bypass modulation hard limits or write raw HRTIM registers.