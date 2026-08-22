# Host Interface and UART Protocol

## Purpose

This document defines the host-to-converter communication boundary for the bidirectional buck-boost control platform.

The host interface is intended for supervisory control, telemetry, experiment automation, and parameter management. It is **not** part of the real-time power-control loop.

The intended host path is:

```text
Web Browser
    ↓
Web Serial API
    ↓
USB-to-UART adapter
    ↓
STM32F334 USART1
    ↓
Power Manager / Telemetry
```

The converter must remain locally deterministic. Voltage/current control, modulation, PWM timing, and protection execute on the STM32F334 and must not depend on host timing.

## Design Principles

1. The host sends **supervisory commands**, not switching commands.
2. The host must never directly command individual MOSFET states or per-cycle duty ratios during normal operation.
3. Every write command is validated by the firmware before it affects the power stage.
4. Safety and fault handling remain authoritative on the converter.
5. The wire protocol is binary, versioned, deterministic, and independent of the Web UI implementation.
6. Engineering values are transferred as fixed-point integers rather than protocol-level floating-point values.
7. The same protocol should be reusable by a future Python CLI, automated test tool, or other host application.

## Physical Interface

The current hardware exposes STM32F334 USART1 through the existing UART header.

Initial protocol settings:

| Parameter | Value |
| --- | --- |
| Interface | USART1 |
| Format | 8 data bits, no parity, 1 stop bit |
| Initial baud rate | 115200 bit/s |
| Flow control | None |
| Duplex | Full duplex |

`115200` is intentionally conservative for first bring-up. Higher rates such as `460800` or `921600` may be introduced after protocol and signal-integrity validation.

## Layering

```text
Web UI
  ↓
Device API
  ↓
Protocol Codec
  ↓
Web Serial Transport
  ↓
UART
  ↓
Firmware Protocol Parser
  ↓
Power Manager / Telemetry Provider
```

The UI must not directly read from or write to the serial port. UI components interact only with the Device API.

## Frame Format

Protocol version 1 uses the following frame:

```text
+--------+--------+---------+------+-----+-------+--------+---------+-------+
| SOF0   | SOF1   | VERSION | TYPE | CMD | SEQ   | LENGTH | PAYLOAD | CRC16 |
| 1 byte | 1 byte | 1 byte  | 1 B  | 1 B | 2 B   | 2 B    | N bytes | 2 B   |
+--------+--------+---------+------+-----+-------+--------+---------+-------+
```

### Fields

| Field | Size | Description |
| --- | ---: | --- |
| `SOF0` | 1 | `0xAA` |
| `SOF1` | 1 | `0x55` |
| `VERSION` | 1 | Protocol version, initially `0x01` |
| `TYPE` | 1 | Message type |
| `CMD` | 1 | Command identifier |
| `SEQ` | 2 | Host-generated sequence number |
| `LENGTH` | 2 | Payload length in bytes |
| `PAYLOAD` | N | Command-specific payload |
| `CRC16` | 2 | CRC-16/CCITT-FALSE over `VERSION` through end of payload |

All multi-byte integer fields use **little-endian** byte order.

Version-1 maximum payload length is `240` bytes. Larger data objects must be transferred using future chunked-transfer commands rather than oversized frames.

## Message Types

| Value | Name | Direction | Meaning |
| ---: | --- | --- | --- |
| `0x01` | `REQUEST` | Host → MCU | Command or query |
| `0x02` | `RESPONSE` | MCU → Host | Response matching `SEQ` |
| `0x03` | `EVENT` | MCU → Host | Asynchronous event, reserved for later use |
| `0x04` | `TELEMETRY` | MCU → Host | Periodic telemetry, reserved for later use |

V0.1 uses request/response operation. Asynchronous telemetry is intentionally deferred until the basic transport is stable.

## Response Status

Every response payload begins with one status byte.

| Value | Name | Meaning |
| ---: | --- | --- |
| `0x00` | `OK` | Command completed successfully |
| `0x01` | `ERR_BAD_CMD` | Unknown or unsupported command |
| `0x02` | `ERR_BAD_LENGTH` | Invalid payload size |
| `0x03` | `ERR_BAD_VALUE` | Value outside allowed range |
| `0x04` | `ERR_BAD_STATE` | Command not allowed in current converter state |
| `0x05` | `ERR_FAULT_ACTIVE` | Command blocked by active fault |
| `0x06` | `ERR_BUSY` | Resource or operation currently busy |
| `0x07` | `ERR_INTERNAL` | Internal firmware error |

## V0.1 Commands

### Command Summary

| ID | Command | Direction | Purpose |
| ---: | --- | --- | --- |
| `0x01` | `PING` | R/W | Verify link and protocol parser |
| `0x02` | `GET_INFO` | Read | Read firmware/protocol identification |
| `0x03` | `GET_STATUS` | Read | Read converter state and measurements |
| `0x10` | `SET_VREF` | Write | Set output-voltage reference |
| `0x11` | `SET_ILIMIT` | Write | Set output-current limit/reference ceiling |
| `0x12` | `OUTPUT_ENABLE` | Write | Request converter startup |
| `0x13` | `OUTPUT_DISABLE` | Write | Request converter shutdown |
| `0x14` | `CLEAR_FAULT` | Write | Request clearing of recoverable faults |

### `PING` — `0x01`

Request payload: empty.

Response payload after status:

```text
uint32_t uptime_ms
```

The host uses `PING` during initial connection and link diagnostics.

### `GET_INFO` — `0x02`

Request payload: empty.

Response payload after status:

```text
uint8_t  protocol_major
uint8_t  protocol_minor
uint8_t  firmware_major
uint8_t  firmware_minor
uint8_t  firmware_patch
uint8_t  controller_capability_bits
uint32_t build_id
```

`build_id` is implementation-defined and may be a compact build number or truncated revision identifier.

### `GET_STATUS` — `0x03`

Request payload: empty.

Response payload after status:

```text
uint32_t uptime_ms
uint8_t  power_state
uint8_t  operating_region
uint8_t  controller_type
uint8_t  status_flags
uint32_t vin_mV
int32_t  iin_mA
uint32_t vout_mV
int32_t  iout_mA
uint32_t vref_mV
uint32_t ilimit_mA
uint32_t fault_bits
uint16_t duty_a_q15
uint16_t duty_b_q15
```

The Web UI may derive quantities such as input power, output power, and efficiency from these primary measurements.

Current values are signed to preserve future bidirectional power-flow semantics.

Duty values use unsigned Q15 scaling:

```text
0x0000 = 0.0
0x7FFF ≈ 1.0
```

### `SET_VREF` — `0x10`

Request payload:

```text
uint32_t vref_mV
```

Response payload: status only.

The firmware validates the requested value against configured operating limits before accepting it.

### `SET_ILIMIT` — `0x11`

Request payload:

```text
uint32_t ilimit_mA
```

Response payload: status only.

The current limit is a supervisory constraint. The exact internal control action is determined by the active control architecture.

### `OUTPUT_ENABLE` — `0x12`

Request payload: empty.

Response payload: status only.

This command is a **request to start**, not a direct PWM-enable operation. The Power Manager remains responsible for qualification, soft-start, fault checks, and state transitions.

### `OUTPUT_DISABLE` — `0x13`

Request payload: empty.

Response payload: status only.

The disable request must be accepted from any non-reset state unless the firmware is already executing a more restrictive safety shutdown.

### `CLEAR_FAULT` — `0x14`

Request payload: empty.

Response payload: status only.

Only recoverable faults may be cleared remotely. Latched or hardware-critical faults may require power cycling or an explicit local recovery procedure.

## State Enumerations

Initial `power_state` values:

| Value | State |
| ---: | --- |
| `0` | `OFF` |
| `1` | `QUALIFY` |
| `2` | `SOFT_START` |
| `3` | `REGULATION` |
| `4` | `FAULT` |

Initial `operating_region` values:

| Value | Region |
| ---: | --- |
| `0` | `UNKNOWN` |
| `1` | `BUCK` |
| `2` | `MIXED` |
| `3` | `BOOST` |

Initial `controller_type` values:

| Value | Controller |
| ---: | --- |
| `0` | `NONE` |
| `1` | `PI` |
| `2` | `DEADBEAT` |
| `3` | `SUPER_TWISTING_SMC` |
| `4` | `LQI` |
| `5` | `MPC` |

The protocol reserves controller identifiers early so the Web UI and firmware can evolve without redesigning the transport.

## Units and Scaling

Protocol fields use explicit engineering-unit scaling.

| Quantity | Wire representation |
| --- | --- |
| Voltage | millivolts (`mV`) |
| Current | milliamps (`mA`) |
| Time | milliseconds (`ms`) unless otherwise documented |
| Duty ratio | unsigned Q15 |
| Power | normally calculated by host from V/I |

Floating-point values are not transferred in V0.1.

## Sequence Handling

The host increments `SEQ` for each request. The MCU copies the request sequence number into the corresponding response.

The host uses sequence numbers to:

- match responses to commands;
- reject stale responses;
- diagnose timeouts;
- support future asynchronous telemetry without ambiguity.

The MCU does not need to process multiple outstanding V0.1 requests concurrently. The initial Web client should use one outstanding request at a time.

## Parser Requirements

The firmware parser must:

1. search for `0xAA 0x55` synchronization bytes;
2. reject unsupported protocol versions;
3. reject payload lengths larger than the configured maximum;
4. wait for the complete frame;
5. validate CRC before dispatch;
6. discard a corrupted frame without changing converter control state;
7. resynchronize on the next valid SOF sequence;
8. never execute a partially received command.

Parsing and command handling must not execute inside the 200 kHz real-time control path.

## Safety Boundary

The UART protocol is intentionally outside the real-time safety boundary.

```text
Host command
    ↓
Protocol validation
    ↓
Power Manager validation
    ↓
Reference / state request
    ↓
Real-time control loop
    ↓
Modulation / PWM
```

The following operations are prohibited through the normal host protocol:

- direct Q1/Q2/Q3/Q4 switching commands;
- host-timed PWM generation;
- disabling mandatory overcurrent or overvoltage protection;
- bypassing soft-start and power-stage qualification;
- commanding values outside firmware-enforced limits.

Communication loss must never disable local hardware or firmware protection.

The policy for whether an already-running converter continues at its last valid reference or shuts down after host loss is intentionally left configurable and will be defined when remote-session behavior is implemented.

## Web Client V0.1 Transaction Model

Initial connection sequence:

```text
User clicks Connect
      ↓
Browser selects serial port
      ↓
Open 115200 8N1
      ↓
PING
      ↓
GET_INFO
      ↓
GET_STATUS
      ↓
Dashboard ready
```

Normal dashboard operation initially polls `GET_STATUS` at a modest rate such as 10–20 Hz. This is sufficient for bring-up and avoids adding unsolicited telemetry before transport behavior is verified.

The later Scope implementation may enable a higher-rate telemetry mode or buffered capture mechanism.

## Planned Extensions

The following features are deliberately outside V0.1 but supported by the protocol architecture:

- periodic telemetry streaming;
- controller selection and tuning parameters;
- programmable voltage/current sequences;
- deterministic profile execution on the MCU;
- waveform capture and chunked data transfer;
- calibration read/write;
- fault-history retrieval;
- capability discovery;
- firmware-update handoff;
- remote-session watchdog policy.

## V0.1 Acceptance Criteria

The protocol is ready for Web UI integration when all of the following are demonstrated:

1. Web Serial connects to the USB-to-UART adapter.
2. `PING` survives repeated connect/disconnect cycles.
3. `GET_INFO` returns valid protocol and firmware identification.
4. `GET_STATUS` returns stable Vin/Vout/Iin/Iout values.
5. CRC-corrupted packets are rejected without side effects.
6. Invalid references are rejected by firmware.
7. `OUTPUT_ENABLE` enters the normal Power Manager startup path.
8. `OUTPUT_DISABLE` safely requests shutdown.
9. Browser loss or UART noise cannot bypass converter protection.
