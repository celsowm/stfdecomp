#!/usr/bin/env python3
"""Inspect the recovered cpres command 0x77 protocol from a SHARC upload.

The command handler is dispatch entry 0x77 at PM 0x20B1F. It consumes four
FIFO words (XYZ + radius), performs two collision-table scans, and emits nine
FIFO words. This tool reports the evidence-backed protocol shape and verifies
the key handler instructions; it intentionally leaves unproven metadata fields
unnamed.
"""

from __future__ import annotations

import argparse
from pathlib import Path

PM_BASE = 0x20000
PACKET_BYTES = 6
HANDLER = 0x20B1F

INPUT_LOADS = {
    0x20B20: ("x", 0x403E04000000),
    0x20B22: ("y", 0x403E04800000),
    0x20B24: ("z", 0x403E05000000),
    0x20B26: ("radius", 0x403E05800000),
}

OUTPUT_SCRATCH = (
    (0, 0x1C, "push_x"),
    (1, 0x1E, "push_z"),
    (2, 0x1A, "meta_1a"),
    (3, 0x19, "meta_19"),
    (4, 0x18, "meta_18"),
    (5, 0x13, "meta_13"),
    (6, 0x15, "meta_15"),
    (7, 0x14, "meta_14"),
    (8, 0x16, "meta_16"),
)



FINE_PHASE_PACKETS = {
    0x20B92: 0x587E00000000,
    0x20B93: 0x587E00882008,
    0x20B94: 0x587E01082119,
    0x20B95: 0x5A7E0308222A,
    0x20B96: 0x013E00040666,
    0x20B99: 0x06BE0402034C,
    0x20B9B: 0xA6030000001B,
    0x20B9C: 0x467E00881663,
    0x20B9D: 0x013E0008A006,
}

BROAD_PHASE_PACKETS = {
    0x20B83: 0xA80000000027,
    0x20B84: 0xA80100000028,
    0x20B85: 0xA80200000029,
    0x20B87: 0x06BE0402034C,
    0x20B8A: 0x0F0340400000,
    0x20B8B: 0x013E0008A003,
    0x20B8C: 0x062400020BBA,
    0x20B91: 0x0C0020000029,
}

SCAN_BASES = (
    (0x20B38, 0x00030600),
    (0x20B4A, 0x00030700),
)


def packet(data: bytes, pm_address: int) -> int:
    index = pm_address - PM_BASE
    if index < 0:
        raise ValueError("PM address before upload base")
    offset = index * PACKET_BYTES
    if offset + PACKET_BYTES > len(data):
        raise ValueError(f"PM 0x{pm_address:X} outside upload")
    return int.from_bytes(data[offset:offset + PACKET_BYTES], "little")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("upload", type=Path)
    args = parser.parse_args()
    data = args.upload.read_bytes()

    print(f"handler: PM 0x{HANDLER:05X}")
    print("inputs:")
    for address, (name, expected) in INPUT_LOADS.items():
        actual = packet(data, address)
        status = "ok" if actual == expected else "FAIL"
        print(
            f"  {name:6s} PM 0x{address:05X} "
            f"packet=0x{actual:012X} {status}"
        )
        if actual != expected:
            return 1

    print("broad phase:")
    for address, expected in BROAD_PHASE_PACKETS.items():
        actual = packet(data, address)
        status = "ok" if actual == expected else "FAIL"
        print(
            f"  PM 0x{address:05X} packet=0x{actual:012X} "
            f"expected=0x{expected:012X} {status}"
        )
        if actual != expected:
            return 2

    print(
        "  semantics: load fighter-root XYZ, compute 3D distance, "
        "reject when distance > 3.0f, then scan 32 collision entries"
    )

    print("fine phase:")
    for address, expected in FINE_PHASE_PACKETS.items():
        actual = packet(data, address)
        status = "ok" if actual == expected else "FAIL"
        print(
            f"  PM 0x{address:05X} packet=0x{actual:012X} "
            f"expected=0x{expected:012X} {status}"
        )
        if actual != expected:
            return 3
    print(
        "  semantics: each scan record supplies center XYZ and radius; "
        "zero radius is skipped, otherwise distance3(query, center) is "
        "compared with query_radius + ball_radius"
    )

    print("collision scan bases:")
    for address, expected_base in SCAN_BASES:
        actual = packet(data, address) & 0xFFFFFFFF
        status = "ok" if actual == expected_base else "FAIL"
        print(
            f"  PM 0x{address:05X} -> 0x{actual:08X} "
            f"expected=0x{expected_base:08X} {status}"
        )
        if actual != expected_base:
            return 4

    print("outputs:")
    for index, scratch, name in OUTPUT_SCRATCH:
        print(f"  word[{index}] <- scratch +0x{scratch:02X} ({name})")

    print(
        "host use: epc_oidasi scales word[0]/word[1] by 0.16 and "
        "adds them to X/Z; projectile code preserves words[2..8]."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
