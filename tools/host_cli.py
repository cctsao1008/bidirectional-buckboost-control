#!/usr/bin/env python3
"""Gate 1 host CLI for the COBS/CRC16 UART protocol."""

from __future__ import annotations

import argparse
import struct
import sys
import time
from dataclasses import dataclass

try:
    import serial  # type: ignore
except ImportError as exc:  # pragma: no cover - dependency check
    raise SystemExit(
        "pyserial is required. Install it with: python3 -m pip install pyserial"
    ) from exc

PROTOCOL_VERSION = 1
TYPE_REQUEST = 0x01
TYPE_RESPONSE = 0x02

CMD_PING = 0x01
CMD_GET_INFO = 0x02
CMD_GET_STATUS = 0x03

STATUS_NAMES = {
    0x00: "OK",
    0x01: "ERR_BAD_CMD",
    0x02: "ERR_BAD_LENGTH",
    0x03: "ERR_BAD_VALUE",
    0x04: "ERR_BAD_STATE",
    0x05: "ERR_FAULT_ACTIVE",
    0x06: "ERR_BUSY",
    0x07: "ERR_INTERNAL",
}

POWER_STATE_NAMES = {0: "OFF"}
OPERATING_REGION_NAMES = {0: "UNKNOWN"}
CONTROLLER_NAMES = {0: "NONE"}


@dataclass
class Frame:
    version: int
    msg_type: int
    command: int
    flags: int
    sequence: int
    payload: bytes


def crc16_ccitt_false(data: bytes) -> int:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def cobs_encode(data: bytes) -> bytes:
    out = bytearray([0])
    code_index = 0
    code = 1

    for byte in data:
        if byte == 0:
            out[code_index] = code
            code_index = len(out)
            out.append(0)
            code = 1
        else:
            out.append(byte)
            code += 1
            if code == 0xFF:
                out[code_index] = code
                code_index = len(out)
                out.append(0)
                code = 1

    out[code_index] = code
    return bytes(out)


def cobs_decode(data: bytes) -> bytes:
    out = bytearray()
    index = 0

    while index < len(data):
        code = data[index]
        if code == 0:
            raise ValueError("invalid zero byte inside COBS frame")
        index += 1
        end = index + code - 1
        if end > len(data):
            raise ValueError("truncated COBS block")
        out.extend(data[index:end])
        index = end
        if code != 0xFF and index < len(data):
            out.append(0)

    return bytes(out)


def encode_frame(frame: Frame) -> bytes:
    header = struct.pack(
        "<BBBBHH",
        frame.version,
        frame.msg_type,
        frame.command,
        frame.flags,
        frame.sequence,
        len(frame.payload),
    )
    raw = header + frame.payload
    raw += struct.pack("<H", crc16_ccitt_false(raw))
    return cobs_encode(raw) + b"\x00"


def decode_frame(encoded: bytes) -> Frame:
    raw = cobs_decode(encoded)
    if len(raw) < 10:
        raise ValueError("frame too short")

    version, msg_type, command, flags, sequence, payload_length = struct.unpack_from(
        "<BBBBHH", raw, 0
    )
    expected = 8 + payload_length + 2
    if len(raw) != expected:
        raise ValueError(f"bad frame length: got {len(raw)}, expected {expected}")

    received_crc = struct.unpack_from("<H", raw, len(raw) - 2)[0]
    calculated_crc = crc16_ccitt_false(raw[:-2])
    if received_crc != calculated_crc:
        raise ValueError(
            f"CRC mismatch: received 0x{received_crc:04X}, expected 0x{calculated_crc:04X}"
        )

    if version != PROTOCOL_VERSION:
        raise ValueError(f"unsupported protocol version {version}")

    return Frame(
        version=version,
        msg_type=msg_type,
        command=command,
        flags=flags,
        sequence=sequence,
        payload=raw[8:-2],
    )


def read_delimited_frame(port: serial.Serial, timeout_s: float) -> bytes:
    deadline = time.monotonic() + timeout_s
    encoded = bytearray()

    while time.monotonic() < deadline:
        byte = port.read(1)
        if not byte:
            continue
        if byte == b"\x00":
            if encoded:
                return bytes(encoded)
            continue
        encoded.extend(byte)
        if len(encoded) > 512:
            raise RuntimeError("received frame exceeds safety limit")

    raise TimeoutError("timed out waiting for response")


def transact(port: serial.Serial, command: int, sequence: int, timeout_s: float) -> Frame:
    request = Frame(PROTOCOL_VERSION, TYPE_REQUEST, command, 0, sequence, b"")
    port.reset_input_buffer()
    port.write(encode_frame(request))
    port.flush()

    response = decode_frame(read_delimited_frame(port, timeout_s))
    if response.msg_type != TYPE_RESPONSE:
        raise RuntimeError(f"unexpected message type 0x{response.msg_type:02X}")
    if response.command != command:
        raise RuntimeError(
            f"response command mismatch: got 0x{response.command:02X}, expected 0x{command:02X}"
        )
    if response.sequence != sequence:
        raise RuntimeError(
            f"sequence mismatch: got {response.sequence}, expected {sequence}"
        )
    if not response.payload:
        raise RuntimeError("response payload does not contain status byte")
    return response


def require_ok(frame: Frame) -> bytes:
    status = frame.payload[0]
    if status != 0:
        name = STATUS_NAMES.get(status, f"UNKNOWN_{status}")
        raise RuntimeError(f"device returned {name} (0x{status:02X})")
    return frame.payload[1:]


def print_ping(payload: bytes) -> None:
    if len(payload) != 4:
        raise RuntimeError(f"PING payload length is {len(payload)}, expected 4")
    uptime_ms = struct.unpack("<I", payload)[0]
    print("PING: OK")
    print(f"uptime_ms: {uptime_ms}")


def print_info(payload: bytes) -> None:
    if len(payload) != 10:
        raise RuntimeError(f"GET_INFO payload length is {len(payload)}, expected 10")
    proto_major, proto_minor, fw_major, fw_minor, fw_patch, caps, build_id = struct.unpack(
        "<BBBBBBI", payload
    )
    print("GET_INFO: OK")
    print(f"protocol: {proto_major}.{proto_minor}")
    print(f"firmware: {fw_major}.{fw_minor}.{fw_patch}")
    print(f"capabilities: 0x{caps:02X}")
    print(f"build_id: 0x{build_id:08X}")


def print_status(payload: bytes) -> None:
    if len(payload) != 40:
        raise RuntimeError(f"GET_STATUS payload length is {len(payload)}, expected 40")

    (
        uptime_ms,
        power_state,
        operating_region,
        controller_type,
        status_flags,
        vin_mv,
        iin_ma,
        vout_mv,
        iout_ma,
        vref_mv,
        ilimit_ma,
        fault_bits,
        duty_a_q15,
        duty_b_q15,
    ) = struct.unpack("<IBBBB IiIiIIIHH".replace(" ", ""), payload)

    print("GET_STATUS: OK")
    print(f"uptime_ms: {uptime_ms}")
    print(f"power_state: {POWER_STATE_NAMES.get(power_state, power_state)}")
    print(f"operating_region: {OPERATING_REGION_NAMES.get(operating_region, operating_region)}")
    print(f"controller: {CONTROLLER_NAMES.get(controller_type, controller_type)}")
    print(f"status_flags: 0x{status_flags:02X}")
    print(f"vin: {vin_mv / 1000.0:.3f} V")
    print(f"iin: {iin_ma / 1000.0:.3f} A")
    print(f"vout: {vout_mv / 1000.0:.3f} V")
    print(f"iout: {iout_ma / 1000.0:.3f} A")
    print(f"vref: {vref_mv / 1000.0:.3f} V")
    print(f"ilimit: {ilimit_ma / 1000.0:.3f} A")
    print(f"fault_bits: 0x{fault_bits:08X}")
    print(f"duty_a_q15: {duty_a_q15}")
    print(f"duty_b_q15: {duty_b_q15}")


def run_self_test() -> None:
    assert crc16_ccitt_false(b"123456789") == 0x29B1
    sample = bytes([0x11, 0x22, 0x00, 0x33, 0x00, 0x44])
    assert cobs_decode(cobs_encode(sample)) == sample
    frame = Frame(1, TYPE_REQUEST, CMD_PING, 0, 0x1234, b"")
    encoded = encode_frame(frame)
    decoded = decode_frame(encoded[:-1])
    assert decoded == frame
    print("host CLI self-test: PASS")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("ping", "info", "status", "self-test"))
    parser.add_argument("--port", help="serial port, for example /dev/ttyUSB0 or COM5")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--timeout", type=float, default=1.0)
    args = parser.parse_args()

    if args.command == "self-test":
        run_self_test()
        return 0

    if not args.port:
        parser.error("--port is required for device commands")

    command_map = {
        "ping": (CMD_PING, print_ping),
        "info": (CMD_GET_INFO, print_info),
        "status": (CMD_GET_STATUS, print_status),
    }
    command, printer = command_map[args.command]

    try:
        with serial.Serial(args.port, args.baud, timeout=0.05) as port:
            response = transact(port, command, sequence=1, timeout_s=args.timeout)
            printer(require_ok(response))
    except (OSError, serial.SerialException, TimeoutError, ValueError, RuntimeError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
