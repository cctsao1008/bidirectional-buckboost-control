# Firmware Architecture

## Purpose

This document defines the implementation baseline for the independent STM32F334 firmware. It describes stable layering, timing, ownership, and safety rules. Current bring-up progress belongs in GitHub Issues rather than in this document.

## Technology Baseline

| Area | Baseline |
| --- | --- |
| MCU | STM32F334C8T6 |
| Peripheral library | libopencm3 |
| Numerical library | CMSIS-DSP where useful |
| RTOS | None initially |
| Scheduling | interrupt-driven hard real-time + cooperative background loop |
| Host transport | USART1 PB6/PB7 |
| Framing | COBS + `0x00` delimiter |
| Integrity | CRC-16/CCITT-FALSE |
| Host clients | CLI first, Web Serial later |

FreeRTOS is not part of the initial architecture. It may be reconsidered only if future firmware gains genuinely concurrent services such as networking, filesystems, or multiple complex communication stacks. The switching-cycle control path remains outside RTOS scheduling even if an RTOS is introduced later.

## Timing Budget

At 200 kHz:

```text
Tsw = 5 us
```

At a 64 MHz Cortex-M4 clock this is approximately 320 core cycles per switching period. That budget must include measurement handoff, fast controller execution, constraints, modulation, HRTIM update, and instrumentation overhead. WCET and deadline misses therefore become first-class metrics before advanced controllers are accepted.

## Timing Classes

```text
Hard real-time
────────────────────────
PWM/HRTIM update
synchronized measurement handoff
fast current/control law
modulation hard limits
critical fault response

Soft real-time
────────────────────────
Power Manager
operating-region management
reference ramps
slow protection
estimator health supervision

Background
────────────────────────
UART protocol
telemetry
CLI/Web requests
diagnostics
parameter management
capture transfer
```

## Interrupt Policy

### Switching-cycle interrupt

The switching-cycle ISR may only perform work that belongs to the fixed deadline:

- consume the latest synchronized measurement set;
- update `iL_hat` where the estimator runs at the fast rate;
- execute the active fast controller;
- apply current/actuation constraints;
- execute unified modulation;
- atomically update HRTIM compare values;
- record bounded timing/capture metadata;
- react to time-critical fault state when required.

It must not allocate memory, block, parse UART, format strings, access host-facing code, or depend on host availability.

### USART1 RX interrupt

The UART ISR only transfers received bytes to an SPSC ring buffer:

```text
USART RXNE IRQ
      ↓
ring_buffer_push_isr()
      ↓
return
```

COBS decoding, CRC checking, dispatch, and response generation remain in background context.

## Main Loop

Conceptually:

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

Periodic background work is released by monotonic time flags rather than blocking delays.

## Layering and Ownership

```text
Application / Power Manager
        ↓
Control / Estimation
        ↓
Unified Modulation
        ↓
Platform API
        ↓
libopencm3 / small STM32F334 helpers
        ↓
STM32F334
```

Host communication is parallel to the real-time path:

```text
CLI / Web App
      ↓
Protocol Codec
      ↓
USART1
      ↓
Power Manager / Telemetry
```

### `firmware/app`

Owns application composition and background services. It does not own direct PWM authority.

### `firmware/control`

Owns estimators and controller implementations. Controllers operate on calibrated physical quantities and logical actuation requests. They never write timer registers.

### `firmware/power`

Owns power-domain data structures such as calibrated measurements, references, derived power quantities, and other hardware-independent converter abstractions.

### `firmware/platform`

Owns STM32F334 clocks, GPIO, UART, ADC/DMA, HRTIM, timing counters, and all direct libopencm3/register access. If libopencm3 lacks required F334 HRTIM coverage, a minimal register-level helper belongs here.

### `firmware/protocol`

Owns MCU-independent COBS framing, CRC, packet format, and stream parsing. It remains host-unit-testable.

### `firmware/safety`

Owns fault evaluation, Power Manager state, qualification, shutdown, and policy that must remain independent of the selected controller.

## Canonical Control Path

All notation follows `control-conventions.md`:

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
outer voltage / energy controller
        ↓
iL_ref
        ↓
inner current controller
        ↓
vL*
        ↓
unified modulation
        ↓
d1 / d2
        ↓
platform HRTIM API
```

The board has no direct `iL` ADC channel and the target architecture does not add one.

## Controller Implementation Order

The roadmap order is:

```text
synchronized measurement
        ↓
iL estimator
        ↓
cascaded voltage/current PI
        ↓
unified vL* -> d1/d2 modulation
        ↓
unified bidirectional operation
        ↓
LQI
        ↓
Deadbeat Predictive Current Control
        ↓
Super-Twisting SMC
        ↓
constrained / reduced MPC
```

This is not a requirement that every controller be implemented before the next can be investigated; it defines dependency order. Advanced control cannot bypass measurement, estimator, modulation, protection, or timing prerequisites.

## HRTIM Ownership Rule

Power-stage GPIOs begin in a safe-low GPIO state. HRTIM configuration does not imply switching authority.

Required ownership sequence:

```text
GPIO safe-low
   ↓
configure HRTIM timebase / polarity / dead time / faults
   ↓
force HRTIM outputs inactive
   ↓
switch pins to HRTIM alternate function
   ↓
verify inactive behavior
   ↓
Power Manager qualification
   ↓
explicit enable
```

## Host Protocol Boundary

The transport path is:

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
```

Protocol corruption must never alter converter state. Write commands become Power Manager/reference requests only after full validation.

The first physical host client is intentionally a simple Windows-side CLI because it isolates UART/protocol bring-up from browser/Web Serial variables. Web Serial is a later experiment-platform layer, not a prerequisite for firmware bring-up.

## Numerical Library Policy

CMSIS-DSP is a primitive library, not the controller architecture. Appropriate uses include filtering, matrices, observers, statistics, and optimized arithmetic. Controller code remains project-owned so saturation, anti-windup, state transfer, instrumentation, and benchmarking are consistent.

## Parameter Update Policy

Control parameters must not change partially during a switching-cycle computation. Host-visible configuration should follow:

```text
shadow parameters
      ↓
validation
      ↓
atomic/safe-point commit
      ↓
active parameters
```

Persistent calibration/configuration should later include versioning, CRC/integrity, and deterministic defaults.

## Safety Rules

1. Host commands are supervisory requests only.
2. `OUTPUT_ENABLE` passes through Power Manager qualification and soft-start.
3. Mandatory protection cannot be bypassed by normal host commands or controller selection.
4. UART loss cannot remove local protection.
5. Gate-driver `DISABLE` is not MCU-controlled on V1.2; firmware must own safe HRTIM forcing correctly.
6. PWM polarity, complementary timing, effective non-overlap, and minimum pulse behavior are verified as implementation deltas before significant bus power is applied.
7. No dynamic allocation, blocking I/O, mutex, logging, or formatting is permitted in the 200 kHz path.
8. Every advanced controller must publish WCET/deadline evidence over its intended operating envelope.
