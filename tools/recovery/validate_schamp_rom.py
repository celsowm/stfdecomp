#!/usr/bin/env python3
"""Validate a local Sonic Championship (schamp) ROM set as secondary STF evidence.

No ROM bytes are stored in the repository. The caller supplies schamp.zip.
The script verifies the known asset CRCs used by tools/sfight_data.json,
reconstructs the interleaved i960 program from epr-19141.15/.16 in memory,
and checks the collision corridor signature observed in Sonic Championship.

This is secondary evidence only. The STF reference program remains the sfight
set (epr-19001.15/.16).
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import zipfile
from pathlib import Path


REQUIRED_CRCS = {
    "mpr-19007.11": 0x8B8FF751,
    "mpr-19008.12": 0xA94654F5,
    "mpr-19005.9": 0x98CD1127,
    "mpr-19006.10": 0xE79F0A26,
    "mpr-19009.17": 0xFD410350,
    "mpr-19012.21": 0x9BB7B5B6,
    "mpr-19010.18": 0x6FD94187,
    "mpr-19013.22": 0x9E232FE5,
    "mpr-19019.27": 0x59121896,
    "mpr-19017.25": 0x7B298379,
    "mpr-19020.28": 0x9540DBA0,
    "mpr-19018.26": 0x3B7E7A12,
}

PROGRAM_PAIR = ("epr-19141.15", "epr-19142.16")

COLI_INIT = 0x000293B8
COLLISION = 0x00029418

# Immediate words present in the recovered coli_init body.
COLI_CONSTANTS = {
    0x000293BC: COLLISION,
    0x000293D0: 0x3C23D70A,
    0x000293E8: 0x3DCCCCCD,
    0x000293F4: 0x3FC00000,
    0x00029400: 0x40400000,
}


def interleave_words(left: bytes, right: bytes) -> bytes:
    if len(left) != len(right) or len(left) % 2 != 0:
        raise ValueError("program EPROM pair has incompatible lengths")

    out = bytearray(len(left) + len(right))
    cursor = 0
    for offset in range(0, len(left), 2):
        out[cursor:cursor + 2] = left[offset:offset + 2]
        out[cursor + 2:cursor + 4] = right[offset:offset + 2]
        cursor += 4
    return bytes(out)


def validate(path: Path) -> int:
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())

        missing = [
            name for name in (*REQUIRED_CRCS, *PROGRAM_PAIR)
            if name not in names
        ]
        if missing:
            print("missing files:")
            for name in missing:
                print(f"  {name}")
            return 2

        print("asset CRCs:")
        for name, expected in REQUIRED_CRCS.items():
            actual = archive.getinfo(name).CRC
            status = "ok" if actual == expected else "FAIL"
            print(f"  {name}: {actual:08x} {status}")
            if actual != expected:
                return 3

        program = interleave_words(
            archive.read(PROGRAM_PAIR[0]),
            archive.read(PROGRAM_PAIR[1]),
        )

    print(f"program size: 0x{len(program):X}")
    print(f"program sha256: {hashlib.sha256(program).hexdigest()}")
    print(f"coli_init: 0x{COLI_INIT:08X}")
    print(f"collision: 0x{COLLISION:08X}")

    for offset, expected in COLI_CONSTANTS.items():
        if offset + 4 > len(program):
            print(f"signature offset outside program: 0x{offset:08X}")
            return 4
        actual = struct.unpack_from("<I", program, offset)[0]
        status = "ok" if actual == expected else "FAIL"
        print(
            f"  [0x{offset:08X}] 0x{actual:08X} "
            f"expected=0x{expected:08X} {status}"
        )
        if actual != expected:
            return 5

    print("schamp collision signature: verified (secondary evidence)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("romset", type=Path, help="path to local schamp.zip")
    args = parser.parse_args()
    return validate(args.romset)


if __name__ == "__main__":
    raise SystemExit(main())
