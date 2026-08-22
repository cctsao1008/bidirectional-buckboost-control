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

1. The host sends supervisory commands, not switching commands.
2. The host must never directly command individual MOSFET states or per-cycle duty ratios during normal operation.
3. Every write command is validated by the firmware before it affects the power stage.
4. Safety and fault handling remain authoritative on the converter.
5. The wire protocol is binary, versioned, deterministic, and independent of the Web UI implementation.
6. COBS provides framing and rapid stream resynchronization; CRC16 provides data-integrity checking.
7. Engineering values are transferred as fixed-point integers rather than protocol-level floating-point values.
8. The same protocol should be reusable by future Web, CLI, test, or analysis clients.

## Physical Interface

The current hardware exposes STM32F334 USART1 through the existing UART header.

| Parameter | Value |
| --- | --- |
| Interface | USART1 |
| MCU pins | PB6 TX / PB7 RX |
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

## Wire Framing

Protocol version 1 uses a binary raw frame, followed by CRC16, COBS encoding, and a `0x00` delimiter.

```text
Raw frame
──────────────────────────────────────────────
VERSION | TYPE | CMD | FLAGS | SEQ | LENGTH
PAYLOAD
CRC16
──────────────────────────────────────────────
                  ↓
                 COBS
                  ↓
encoded bytes ... 0x00
                  ^ frame delimiter
```

The raw packet layout is:

```text
+---------+------+-----+-------+--------+--------+---------+-------+
| VERSION | TYPE | CMD | FLAGS | SEQ    | LENGTH | PAYLOAD | CRC16 |
| 1 byte  | 1 B  | 1 B | 1 B   | 2 B    | 2 B    | N bytes | 2 B   |
+---------+------+-----+-------+--------+--------+---------+-------+
```

### Fields

| Field | Size | Description |
| --- | ---: | --- |
| `VERSION` | 1 | Protocol major version, initially `0x01` |
| `TYPE` | 1 | Message type |
| `CMD` | 1 | Command identifier |
| `FLAGS` | 1 | Reserved for command/transport flags; initially zero |
| `SEQ` | 2 | Host-generated sequence number |
| `LENGTH` | 2 | Payload length in bytes |
| `PAYLOAD` | N | Command-specific payload |
| `CRC16` | 2 | CRC-16/CCITT-FALSE over `VERSION` through end of payload |

All multi-byte integer fields use little-endian byte order.

Version-1 maximum payload length is `240` bytes. Larger objects must use future chunked-transfer commands rather than oversized frames.

### COBS framing

The complete raw frame, including CRC16, is COBS encoded. A single `0x00` byte terminates every encoded frame.

Properties important to this project:

- encoded frame data never contains `0x00`;
- one delimiter unambiguously marks a frame boundary;
- a damaged frame can be discarded without scanning for multi-byte SOF patterns;
- the next `0x00` delimiter provides a deterministic resynchronization point;
- framing overhead is bounded and small.

COBS does not provide corruption detection. CRC16 remains mandatory.

### CRC

Protocol version 1 uses CRC-16/CCITT-FALSE:

```text
Polynomial : 0x1021
Init       : 0xFFFF
RefIn      : false
RefOut     : false
XorOut     : 0x0000
Check      : 0x29B1 for "123456789"
```

CRC is calculated over the raw frame from `VERSION` through the last payload byte, before COBS encoding.

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

The Web UI may derive input power, output power, and efficiency from these primary measurements.

Current values are signed to preserve bidirectional power-flow semantics.

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

This command is a request to start, not a direct PWM-enable operation. The Power Manager remains responsible for qualification, soft-start, fault checks, and state transitions.

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

These identifiers reserve protocol space; they do not imply that all controllers are already implemented or validated.

## Units and Scaling

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

The MCU does not need to process multiple outstanding V0.1 requests concurrently. The initial Web client should use one outstanding request at a time.

## Firmware Receive Path

The initial firmware path is:

```text
USART RX interrupt
      ↓
byte ring buffer
      ↓
background protocol service
      ↓
collect bytes until 0x00
      ↓
COBS decode
      ↓
length / version checks
      ↓
CRC16 check
      ↓
command dispatch
```

The USART ISR only moves bytes into the ring buffer. COBS decoding, CRC validation, command execution, and response creation must not run in the interrupt handler or in the 200 kHz control path.

### Stream parser requirements

The parser must:

1. treat `0x00` as the only frame delimiter;
2. discard empty frames;
3. drop an overlength encoded frame until the next delimiter;
4. COBS-decode only complete delimited frames;
5. reject unsupported protocol versions;
6. reject payload lengths larger than the configured maximum;
7. validate CRC before command dispatch;
8. discard corrupted frames without changing converter state;
9. never execute a partially received command;
10. resynchronize at the next delimiter after framing damage.

Protocol diagnostics should track at least:

```text
valid_frames
cobs_errors
crc_errors
length_errors
version_errors
rx_overflows
```

## Safety Boundary

The UART protocol is outside the real-time safety boundary.

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

The normal host protocol must not provide:

- direct individual MOSFET switching commands;
- host-timed PWM generation;
- commands that bypass mandatory overcurrent or overvoltage protection;
- commands that bypass soft-start and power-stage qualification;
- references outside firmware-enforced limits.

Communication loss must never disable local protection.

The later remote-session design may support both autonomous operation and a remote-armed mode with controlled shutdown on heartbeat loss.

## Web Client V0.1 Transaction Model

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

Normal dashboard operation initially polls `GET_STATUS` at approximately 10–20 Hz. This is sufficient for bring-up and keeps unsolicited traffic out of the first transport implementation.

High-rate transient measurements must later use MCU-timestamped buffered capture rather than browser timing.

## Planned Extensions

- periodic telemetry streaming;
- capability discovery;
- controller selection and atomic tuning-parameter updates;
- programmable voltage/current sequences;
- deterministic profile execution on the MCU;
- MCU-timestamped waveform capture and chunked data transfer;
- calibration read/write;
- fault-history retrieval;
- remote-session watchdog policy;
- firmware-update handoff.

## V0.1 Acceptance Criteria

The protocol is ready for Web UI integration when all of the following are demonstrated:

1. Web Serial connects to the USB-to-UART adapter.
2. `PING` survives repeated connect/disconnect cycles.
3. `GET_INFO` returns valid protocol and firmware identification.
4. `GET_STATUS` returns stable Vin/Vout/Iin/Iout values.
5. COBS framing resynchronizes after injected byte errors or garbage frames.
6. CRC-corrupted packets are rejected without side effects.
7. Invalid references are rejected by firmware.
8. `OUTPUT_ENABLE` enters the normal Power Manager startup path rather than directly enabling PWM.
9. `OUTPUT_DISABLE` safely requests shutdown.
10. Browser loss or UART noise cannot bypass converter protection.
