# Firmware Architecture

## Purpose

This document defines the implementation baseline for the independent STM32F334 firmware.

The firmware is designed as a deterministic bare-metal digital-power controller with a separate host-supervisory interface. The architecture intentionally keeps hard real-time control independent of Web, UART, logging, and other noncritical activities.

## Technology Baseline

| Area | Baseline |
| --- | --- |
| MCU | STM32F334C8T6 |
| Peripheral library | libopencm3 |
| Numerical library | CMSIS-DSP where useful |
| RTOS | None initially |
| Scheduling | Interrupt-driven hard real-time + cooperative background scheduler |
| Host transport | USART1 on PB6/PB7 |
| Framing | COBS with `0x00` delimiter |
| Integrity | CRC-16/CCITT-FALSE |
| Host | Web Serial application |

FreeRTOS is intentionally not part of the initial architecture. It may be reconsidered only if future firmware grows into genuinely concurrent subsystems such as networking, filesystems, storage, or multiple complex communication stacks.

## Timing Classes

The firmware separates execution by timing criticality.

```text
Hard real-time
────────────────────────
HRTIM / PWM update
ADC synchronization
fast control law
critical fault response

Soft real-time
────────────────────────
power manager
state machine
mode management
slow protection
reference management

Background
────────────────────────
UART protocol
telemetry
Web commands
diagnostics
parameter management
```

The 200 kHz switching period is 5 µs. At a 64 MHz CPU clock this corresponds to roughly 320 core clock cycles per switching period, so execution-time measurement is mandatory for advanced controllers.

## Interrupt Policy

Interrupt service routines must be short and deterministic.

### HRTIM / control interrupt

Responsible only for operations that belong to the switching-cycle deadline, such as:

- consuming synchronized measurements;
- executing the active fast control law;
- applying current/duty constraints;
- executing modulation;
- updating HRTIM compare values;
- handling time-critical fault forcing where available.

The control ISR must not:

- allocate memory;
- block;
- parse UART packets;
- perform logging or formatting;
- call Web/host-facing code;
- depend on host communication.

### USART1 RX interrupt

The USART RX ISR only transfers received bytes into a single-producer/single-consumer ring buffer.

```text
USART RXNE IRQ
      ↓
ring_buffer_push_isr()
      ↓
return from interrupt
```

COBS decoding, CRC checking, command dispatch, and response generation execute in background context.

## Main Loop

The initial firmware uses a cooperative background loop.

Conceptually:

```text
while (1)
{
    host_protocol_service();
    power_manager_service();
    slow_protection_service();
    telemetry_service();
    diagnostics_service();
}
```

Periodic work may be released using timer/SysTick flags rather than RTOS tasks.

## Layering

```text
Application / Power Manager
        ↓
Control / Estimation
        ↓
Modulation
        ↓
Platform API
        ↓
libopencm3
        ↓
STM32F334
```

Host communication is parallel to, not inside, the control path:

```text
Web App
   ↓
Web Serial
   ↓
USB-to-UART
   ↓
USART1
   ↓
COBS + CRC protocol
   ↓
Power Manager / Telemetry
```

## Platform Layer

Only the platform layer should depend directly on libopencm3 register/peripheral APIs.

Initial platform responsibilities include:

- system clock configuration;
- GPIO;
- USART1;
- ADC triggering/acquisition;
- HRTIM/PWM;
- timing/cycle measurement;
- local keys/LEDs where needed.

If a required STM32F334 HRTIM feature is not fully covered by libopencm3, a small register-level implementation may be added inside the platform layer rather than leaking hardware details into control code.

## Protocol Layer

The protocol layer is MCU-independent C code and should remain host-testable.

Current implementation components:

```text
firmware/protocol/
├── cobs.c / cobs.h
├── crc16.c / crc16.h
└── protocol.c / protocol.h
```

Receive path:

```text
USART IRQ
    ↓
RX ring buffer
    ↓
protocol stream collector
    ↓
0x00 delimiter
    ↓
COBS decode
    ↓
version / length validation
    ↓
CRC16 validation
    ↓
command dispatch
```

Protocol corruption must never alter converter control state.

## Control Layer

The controller must not write HRTIM registers directly.

Intended long-term structure:

```text
Measurements
    ↓
Calibration
    ↓
State reconstruction
    ↓
Controller
    ↓
physical/normalized control request
    ↓
Unified modulation
    ↓
Platform PWM API
```

The hardware provides Vin, Iin, Vout, and Iout sensing but no dedicated main-inductor current ADC channel. No additional current sensor is part of the target architecture. Controllers that require `iL` must use a reconstructed state `iL_hat`.

## Numerical Library Policy

CMSIS-DSP is treated as a numerical primitive library, not as the control architecture.

Suitable uses include:

- filtering;
- matrix operations;
- state observers;
- optimized arithmetic;
- statistics and signal analysis where appropriate.

Controller implementations remain project-owned so that saturation, anti-windup, state transfer, instrumentation, and benchmarking are consistent across algorithms.

## Controller Roadmap

The implementation order is driven by hardware observability and computational feasibility:

```text
reference voltage-loop baseline
        ↓
current observability / state reconstruction
        ↓
cascaded voltage-current PI
        ↓
LQI + observer
        ↓
unified modulation refinement
        ↓
Super-Twisting SMC
        ↓
deadbeat predictive current control
        ↓
explicit / reduced predictive MPC
```

Generic online constrained QP-based MPC at the 200 kHz switching rate is not an initial target for the STM32F334.

## Safety Rules

1. Host commands are supervisory requests only.
2. `OUTPUT_ENABLE` must enter Power Manager qualification and soft-start; it must never directly enable PWM.
3. Mandatory protection cannot be disabled through the normal host protocol.
4. UART loss must not remove local protection.
5. Safe HRTIM output forcing must be validated before closed-loop power testing.
6. The gate-driver `DISABLE` pins are not MCU-controlled on the V1.2 hardware, so firmware safety cannot assume a dedicated driver-disable line.
7. PWM polarity, complementary timing, dead time, and bootstrap constraints must be proven at low risk before applying significant bus power.

## Current Bring-Up Gate

The first firmware gate is host communication with the power stage inactive.

```text
USART1 RX/TX
      ↓
RX ring buffer
      ↓
COBS framing
      ↓
CRC16 validation
      ↓
PING / GET_INFO / GET_STATUS
```

Acceptance for this gate does not require PWM operation.

The initial objective is to prove a robust Browser ↔ USB-UART ↔ STM32F334 communication path before integrating power-control commands.
