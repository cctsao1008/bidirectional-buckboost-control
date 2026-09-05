# Host Interface and UART Protocol

## Purpose

This document defines the host-to-converter communication boundary. The host is supervisory and is never part of switching-cycle timing or raw PWM authority.

## Physical interface

| Parameter | Value |
| --- | --- |
| Interface | USART1 |
| Pins | PB6 TX / PB7 RX |
| Baud | 115200 bit/s |
| Format | 8N1 |
| Flow control | none |
| Duplex | full |

## Layering

```text
Host client
    ↓
Device API / protocol codec
    ↓
serial transport
    ↓
USART1
    ↓
firmware protocol parser
    ↓
Power Manager / telemetry provider
```

The host cannot directly write gate states, HRTIM compare registers, or switching-cycle duty commands.

## Wire framing

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

All multi-byte integers are little-endian. Maximum version-1 payload is 240 bytes.

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

## Message types

| Value | Name | Direction |
| ---: | --- | --- |
| `0x01` | `REQUEST` | host -> MCU |
| `0x02` | `RESPONSE` | MCU -> host |
| `0x03` | `EVENT` | MCU -> host |
| `0x04` | `TELEMETRY` | MCU -> host |

Every response payload begins with one status byte.

## Response status

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

## Command namespace

| ID | Command | Semantics |
| ---: | --- | --- |
| `0x01` | `PING` | link/parser check |
| `0x02` | `GET_INFO` | protocol/firmware identity |
| `0x03` | `GET_STATUS` | state and primary measurements |
| `0x10` | `SET_VREF` | reserved voltage-reference request |
| `0x11` | `SET_ILIMIT` | reserved current-limit request |
| `0x12` | `OUTPUT_ENABLE` | reserved Power Manager enable request |
| `0x13` | `OUTPUT_DISABLE` | reserved controlled-shutdown request |
| `0x14` | `CLEAR_FAULT` | reserved recoverable-fault clear request |

Implemented command handlers are:

```text
PING
GET_INFO
GET_STATUS
```

Reserved command IDs are protocol definitions, not raw control authority. An unsupported reserved command returns `ERR_BAD_CMD`.

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

## `GET_STATUS` — `0x03`

Request payload: empty.

Response after status:

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

Current fields are signed according to `control-conventions.md`.

## Power-state values

| Value | State |
| ---: | --- |
| `0` | `OFF` |
| `1` | `QUALIFY` |
| `2` | `SOFT_START` |
| `3` | `REGULATION` |
| `4` | `SHUTDOWN` |
| `5` | `FAULT` |
| `6` | `RETRY_WAIT` |

`protection-and-state-machine.md` owns state semantics.

## Operating-region values

| Value | Region |
| ---: | --- |
| `0` | `UNKNOWN` |
| `1` | `BUCK` |
| `2` | `MIXED` |
| `3` | `BOOST` |

Region is a modulation description, not power-flow direction.

## Controller-type values

| Value | Controller |
| ---: | --- |
| `0` | `NONE` |
| `1` | `CASCADED_PI` |
| `2` | `LQI` |
| `3` | `DEADBEAT` |
| `4` | `SUPER_TWISTING_SMC` |
| `5` | `MPC` |

These values define the wire namespace and do not imply that every controller is present in a given firmware build. Capability bits and firmware identity determine supported functions.

## Write-command safety semantics

Any accepted write command follows this ownership chain:

```text
wire request
    ↓
frame / CRC validation
    ↓
value / range validation
    ↓
Power Manager or configuration request
    ↓
validated safe-point commit
```

`OUTPUT_ENABLE` maps to an enable request entering `QUALIFY`; it does not assert HRTIM enable directly. `OUTPUT_DISABLE` maps to `SHUTDOWN`. `CLEAR_FAULT` invokes fault-policy evaluation and cannot bypass a latched condition.

Direction changes never swap ADC channels or silently redefine `Vin`/`Vout`.

## Sequence handling

The host increments `SEQ`; the MCU copies the sequence into the matching response. Version 1 supports one outstanding request per simple client session.

## Receive path

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

Malformed frames are discarded without changing converter state. The parser resynchronizes at the next delimiter.

## Host client

`tools/host_cli.py` implements the host-side serial client for the protocol foundation and the implemented read-only command set.

## Transport invariants

1. Communication loss cannot remove local protection or PWM shutdown authority.
2. Malformed COBS, length, version, or CRC data has no converter-state side effect.
3. Host requests never bypass Power Manager qualification.
4. High-rate measurement timing is MCU-defined; serial-host timing is not a control or capture clock.