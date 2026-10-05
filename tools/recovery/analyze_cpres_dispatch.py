#!/usr/bin/env python3
"""Inspect the embedded STF Model 2B cpres SHARC command dispatch table.

Input is the raw 0x741C-byte upload produced by extract_cpres_program.py.
The boot code builds a 136-entry table in DM 0x30000; each entry contains the
program-memory handler address for the corresponding cpres command number.
For a correctly extracted upload the table begins at SHARC packet 0xA7.

This deliberately decodes only the evidence-backed dispatch construction, not
arbitrary SHARC instructions.
"""

from __future__ import annotations

import argparse
from pathlib import Path

SHARC_PACKET_BYTES = 6
DISPATCH_PACKET_START = 0xA7
DISPATCH_COUNT = 136
DISPATCH_DM_BASE = 0x30000

KNOWN = {
    0x24: "scaled_sin",
    0x25: "scaled_cos",
    0x27: "ring_angle",
    0x29: "point_transform",
    0x77: "collision_pushout_query",
}


def packet(data: bytes, index: int) -> int:
    offset = index * SHARC_PACKET_BYTES
    if offset + SHARC_PACKET_BYTES > len(data):
        raise ValueError(f"packet 0x{index:X} outside upload")
    return int.from_bytes(data[offset:offset + SHARC_PACKET_BYTES], "little")


def dispatch_handlers(data: bytes) -> list[int]:
    end = DISPATCH_PACKET_START + DISPATCH_COUNT
    if end * SHARC_PACKET_BYTES > len(data):
        raise ValueError("dispatch table outside upload")
    return [
        packet(data, DISPATCH_PACKET_START + i) & 0xFFFFFFFF
        for i in range(DISPATCH_COUNT)
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upload", type=Path)
    parser.add_argument(
        "--command",
        type=lambda value: int(value, 0),
        help="show only one command number",
    )
    args = parser.parse_args()

    data = args.upload.read_bytes()
    handlers = dispatch_handlers(data)

    commands = (
        [args.command]
        if args.command is not None
        else range(DISPATCH_COUNT)
    )
    for command in commands:
        if command < 0 or command >= DISPATCH_COUNT:
            raise SystemExit(f"command 0x{command:X} outside dispatch table")
        handler = handlers[command]
        label = KNOWN.get(command, "")
        suffix = f" {label}" if label else ""
        print(
            f"0x{command:02X} DM[0x{DISPATCH_DM_BASE + command:05X}] "
            f"-> PM 0x{handler:05X}{suffix}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
