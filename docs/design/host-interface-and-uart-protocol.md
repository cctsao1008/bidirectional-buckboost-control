# Host Interface and UART Protocol

## Purpose

This document defines the host-to-converter communication boundary. The host is supervisory; it is never part of the switching-cycle control loop.

The protocol is designed to support both a simple bring-up CLI and the later Web Serial experiment application.

## Physical Interface

| Parameter | Value |
| --- | --- |
| Interface | USART1 |
| Pins | PB6 TX / PB7 RX |
| Initial baud | 115200 bit/s |
| Format | 8N1 |
| Flow control | none |
| Duplex | full |

Higher baud rates may be introduced only after transport and signal-integrity validation.

## Layering

```text
CLI / Web UI
    ↓
Device API / Protocol Codec
    ↓
Serial transport
    ↓
USART1
    ↓
Firmware protocol parser
    ↓
Power Manager / Telemetry provider
```

The host never directly writes gate states, HRTIM compare values, or switching-cycle duties.

## Wire Framing

Protocol major version 1 uses:

```text
raw header + payload + CRC16
        ↓
COBS encode
        ↓
0x00 delimiter
```

Raw layout:

```text
+---------+------+-----+-------+------+--------+---------+-------+
| VERSION | TYPE | CMD | FLAGS | SEQ  | LENGTH | PAYLOAD | CRC16 |
| 1 byte  | 1 B  | 1 B | 1 B   | 2 B | 2 B    | N bytes | 2 B   |
+---------+------+-----+-------+------+--------+---------+-------+
```

All multi-byte integers are little-endian. Maximum payload for version 1 is 240 bytes.

## CRC

CRC-16/CCITT-FALSE:

```text
Polynomial : 0x1021
Init       : 0xFFFF
RefIn      : false
RefOut     : false
XorOut     : 0x0000
Check      : 0x29B1 for "123456789"
```

CRC covers `VERSION` through the final payload byte before COBS encoding.

## Message Types

| Value | Name | Direction |
| ---: | --- | --- |
| `0x01` | `REQUEST` | host -> MCU |
| `0x02` | `RESPONSE` | MCU -> host |
| `0x03` | `EVENT` | MCU -> host, reserved |
| `0x04` | `TELEMETRY` | MCU -> host, reserved |

Every response payload begins with one status byte.

## Response Status

| Value | Name |
| ---: | --- |
| `0x00` | `OK` |
| `0x01` | `ERR_BAD_CMD` |
| `0x02` | `ERR_BAD_LENGTH` |
| `0x03` | `ERR_BAD_VALUE` |
| `0x04` | `ERR_BAD_STATE` |
| `0x05` | `ERR_FAULT_ACTIVE` |
| `0x06` | `ERR_BUSY` |
| `0x07` | `ERR_INTERNAL` |

## Command Namespace

The protocol header reserves these command IDs:

| ID | Command | Intended purpose |
| ---: | --- | --- |
| `0x01` | `PING` | link/parser check |
| `0x02` | `GET_INFO` | protocol/firmware identity |
| `0x03` | `GET_STATUS` | state and primary measurements |
| `0x10` | `SET_VREF` | future regulated-voltage reference request |
| `0x11` | `SET_ILIMIT` | future current-limit/reference ceiling request |
| `0x12` | `OUTPUT_ENABLE` | future Power Manager startup request |
| `0x13` | `OUTPUT_DISABLE` | future controlled-shutdown request |
| `0x14` | `CLEAR_FAULT` | future recoverable-fault clear request |

### Currently implemented subset

The current firmware intentionally implements only:

```text
PING
GET_INFO
GET_STATUS
```

All other reserved commands return `ERR_BAD_CMD` until the corresponding Power Manager/reference functionality exists. Reserving an ID does **not** mean that the feature is implemented or safe to use.

This distinction prevents documentation from implying power-control capability before the firmware actually has it.

## `PING` — `0x01`

Request payload: empty.

Response after status:

```text
uint32_t uptime_ms
```

## `GET_INFO` — `0x02`

Request payload: empty.

Response after status:

```text
uint8_t  protocol_major
uint8_t  protocol_minor
uint8_t  firmware_major
uint8_t  firmware_minor
uint8_t  firmware_patch
uint8_t  capability_bits
uint32_t build_id
```

`build_id` is implementation-defined until a stronger firmware provenance scheme is frozen.

## `GET_STATUS` — `0x03`

Request payload: empty.

Current response after status:

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

Current is signed to preserve bidirectional semantics. `vin_mV` / `vout_mV` retain board-signal naming; UI may display Port A / Port B where direction-neutral terminology is clearer.

The present firmware returns a safe inactive state and zero measurement fields until acquisition is integrated.

## Canonical Power-State Values

Power-state semantics are owned by `protection-and-state-machine.md` and mirrored on the wire:

| Value | State |
| ---: | --- |
| `0` | `OFF` |
| `1` | `QUALIFY` |
| `2` | `SOFT_START` |
| `3` | `REGULATION` |
| `4` | `SHUTDOWN` |
| `5` | `FAULT` |
| `6` | `RETRY_WAIT` |

Boot/reset is not a stable externally visible power state.

## Operating-Region Values

| Value | Region |
| ---: | --- |
| `0` | `UNKNOWN` |
| `1` | `BUCK` |
| `2` | `MIXED` |
| `3` | `BOOST` |

Region describes modulation behavior, not power-flow direction.

## Controller-Type Values

Reserved identifiers:

| Value | Controller |
| ---: | --- |
| `0` | `NONE` |
| `1` | `CASCADED_PI` |
| `2` | `LQI` |
| `3` | `DEADBEAT` |
| `4` | `SUPER_TWISTING_SMC` |
| `5` | `MPC` |

These are namespace reservations, not implementation claims.

## Future Bidirectional Reference Semantics

The existing names `SET_VREF`, `vin_mV`, and `vout_mV` reflect board/history naming. Before enabling bidirectional reference-setting commands, the firmware must freeze a direction-neutral request model that identifies at least:

```text
regulated physical port / objective
requested power-flow direction
voltage/current/power reference as applicable
limits
```

A reverse-power request must **not** be implemented by swapping ADC channels or silently redefining `Vin` and `Vout`.

If the existing `SET_VREF` ID is used, its semantics must be defined relative to an explicit active regulation target/capability, not permanently interpreted as “always regulate physical Port B.” A version/capability extension is preferred over ambiguous semantics.

## Future Write-Command Safety

When implemented:

```text
wire request
    ↓
frame/CRC validation
    ↓
value/range validation
    ↓
shadow request/config
    ↓
Power Manager qualification
    ↓
atomic safe-point commit
    ↓
controller/reference layer
```

`OUTPUT_ENABLE` must request `OFF -> QUALIFY`; it never directly enables HRTIM outputs. `OUTPUT_DISABLE` requests `SHUTDOWN`. `CLEAR_FAULT` requests policy evaluation and cannot bypass a latched fault.

## Sequence Handling

Host increments `SEQ`; the MCU copies the sequence into the matching response. Version 1 assumes at most one outstanding request from the simple client.

## Receive Path

```text
USART RX IRQ
      ↓
ring buffer
      ↓
background stream collector
      ↓
0x00 delimiter
      ↓
COBS decode
      ↓
version / length checks
      ↓
CRC16 check
      ↓
dispatch
```

The parser must discard malformed frames without changing converter state and resynchronize at the next delimiter.

Recommended counters:

```text
rx_bytes
tx_bytes
valid_frames
cobs_errors
crc_errors
length_errors
version_errors
rx_overflows
unknown_commands
```

## Bring-Up Client Strategy

Physical UART/protocol bring-up should use the Windows-side `tools/host_cli.py` first because it isolates serial/protocol behavior from browser/Web Serial complexity:

```text
PING
GET_INFO
GET_STATUS
```

Once this transport is stable on hardware, the Web Serial client can reuse the same wire protocol.

## Web Client Architecture

Later:

```text
User Connect
    ↓
Web Serial open
    ↓
PING
    ↓
GET_INFO / capabilities
    ↓
GET_STATUS
    ↓
Dashboard / experiment services
```

Normal dashboard telemetry may begin as 10–20 Hz polling. High-rate transients must use MCU-timestamped local capture and chunked transfer rather than browser timing.

## Planned Extensions

- capability discovery;
- direction/regulated-port metadata;
- telemetry streaming;
- atomic controller/config updates;
- programmable sequences;
- local deterministic profile execution;
- timestamped waveform capture;
- calibration read/write;
- fault-history retrieval;
- remote-armed heartbeat policy;
- firmware provenance/update handoff.

## Transport Acceptance

The host-transport foundation is accepted when:

1. repeated connect/disconnect preserves parser operation;
2. `PING` responds with valid sequence/CRC framing;
3. `GET_INFO` reports valid protocol/firmware identity;
4. `GET_STATUS` returns a correctly decoded inactive state before measurement integration;
5. malformed COBS/CRC frames are rejected without side effects;
6. stream resynchronization works after garbage/overflow;
7. communication failure cannot alter local protection or PWM authority.

Power-control command acceptance is a later Power Manager milestone, not part of the transport gate.
