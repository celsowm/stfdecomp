#!/usr/bin/env python3
"""Extract the embedded Model 2B cpres SHARC program from an STF ROM set.

The i960 boot loader copies 0x3A0E 16-bit words starting at _cpres_data into
SHARC program memory.  This tool reconstructs the interleaved i960 program from
an sfight/schamp EPROM pair, slices that exact upload image, and can emit either
raw upload bytes or 48-bit SHARC instruction packets (three 16-bit words).

No ROM data is stored in the repository.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

CPRES_PROGRAM_START = 0x000B6318
CPRES_PROGRAM_SIZE = 0x0000741C
SHARC_INSTRUCTION_BYTES = 6


def interleave_words(left: bytes, right: bytes) -> bytes:
    if len(left) != len(right) or len(left) % 2:
        raise ValueError("program EPROM pair has incompatible lengths")
    out = bytearray(len(left) + len(right))
    cursor = 0
    for offset in range(0, len(left), 2):
        out[cursor:cursor + 2] = left[offset:offset + 2]
        out[cursor + 2:cursor + 4] = right[offset:offset + 2]
        cursor += 4
    return bytes(out)


def extract(romset: Path, left_name: str, right_name: str) -> bytes:
    with zipfile.ZipFile(romset) as archive:
        program = interleave_words(
            archive.read(left_name),
            archive.read(right_name),
        )
    end = CPRES_PROGRAM_START + CPRES_PROGRAM_SIZE
    if end > len(program):
        raise ValueError("embedded cpres program falls outside reconstructed image")
    return program[CPRES_PROGRAM_START:end]


def write_packets(data: bytes, output: Path) -> None:
    if len(data) % SHARC_INSTRUCTION_BYTES:
        raise ValueError("cpres upload is not aligned to 48-bit SHARC packets")
    with output.open("w", encoding="utf-8") as stream:
        for index in range(0, len(data), SHARC_INSTRUCTION_BYTES):
            a, b, c = struct.unpack_from("<HHH", data, index)
            packet = a | (b << 16) | (c << 32)
            stream.write(f"{index // 6:04X} {packet:012X} {a:04X} {b:04X} {c:04X}\n")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("romset", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--left", default="epr-19141.15")
    parser.add_argument("--right", default="epr-19142.16")
    parser.add_argument(
        "--packets",
        action="store_true",
        help="write one reconstructed 48-bit SHARC packet per text line",
    )
    args = parser.parse_args()

    data = extract(args.romset, args.left, args.right)
    digest = hashlib.sha256(data).hexdigest()
    print(
        f"cpres bytes={len(data)} instructions={len(data) // 6} "
        f"sha256={digest}"
    )

    if args.packets:
        write_packets(data, args.output)
    else:
        args.output.write_bytes(data)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
