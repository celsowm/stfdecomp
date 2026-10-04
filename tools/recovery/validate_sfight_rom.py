#!/usr/bin/env python3
"""Validate the reference Japanese Sonic the Fighters sfight ROM set.

No ROM bytes are stored in the repository. The caller supplies sfight.zip.
The script verifies the exact CRCs used by tools/sfight_data.json, reconstructs
rom_code1.bin in memory, and validates reference collision/attack corridors.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
import zipfile
from pathlib import Path

REQUIRED_CRCS = {
    "epr-19001.15": 0x9B088511,
    "epr-19002.16": 0x46F510DA,
    "epr-19003.7": 0x63BAE5C5,
    "epr-19004.8": 0xC10C9F39,
}

PROGRAM_PAIR = ("epr-19001.15", "epr-19002.16")

COLI_INIT = 0x0002938C
COLLISION = 0x000293EC

COLI_CONSTANTS = {
    0x00029390: COLLISION,
    0x000293A4: 0x3C23D70A,
    0x000293BC: 0x3DCCCCCD,
    0x000293C8: 0x3FC00000,
    0x000293D4: 0x40400000,
}

ATTACK_HIT_PREFIX_START = 0x0002A858
ATTACK_HIT_PREFIX_END = 0x0002A8F8
ATTACK_HIT_PREFIX_SHA256 = (
    "d12144a0feef70647c53f8f1fb79040032f8875a2f1080f2b34e4a131fbb043d"
)

ATTACK_HIT_STRENGTH_START = 0x0002A8F8
ATTACK_HIT_STRENGTH_END = 0x0002A9F0
ATTACK_HIT_STRENGTH_SHA256 = (
    "25312023a692dad8587001c48bb78a94096c9362fa5ea9217d4d5cbae129c0ca"
)

ATTACK_HIT_MOTION_VECTOR_START = 0x0002B738
ATTACK_HIT_MOTION_VECTOR_END = 0x0002B898
ATTACK_HIT_MOTION_VECTOR_SHA256 = (
    "689813368538becefc91d2de834e4e51c952c4724ca2aee6dabebb336090479f"
)


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


def validate(path: Path) -> int:
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        missing = [name for name in REQUIRED_CRCS if name not in names]
        if missing:
            print("missing files:", ", ".join(missing))
            return 2

        for name, expected in REQUIRED_CRCS.items():
            actual = archive.getinfo(name).CRC
            status = "ok" if actual == expected else "FAIL"
            print(f"{name}: {actual:08x} {status}")
            if actual != expected:
                return 3

        program = interleave_words(
            archive.read(PROGRAM_PAIR[0]),
            archive.read(PROGRAM_PAIR[1]),
        )

    print(f"program sha256: {hashlib.sha256(program).hexdigest()}")
    print(f"coli_init: 0x{COLI_INIT:08X}")
    print(f"collision: 0x{COLLISION:08X}")

    for offset, expected in COLI_CONSTANTS.items():
        actual = struct.unpack_from("<I", program, offset)[0]
        status = "ok" if actual == expected else "FAIL"
        print(
            f"[0x{offset:08X}] 0x{actual:08X} "
            f"expected=0x{expected:08X} {status}"
        )
        if actual != expected:
            return 4

    digest = hashlib.sha256(
        program[ATTACK_HIT_PREFIX_START:ATTACK_HIT_PREFIX_END]
    ).hexdigest()
    print(f"attack_hit prefix sha256={digest}")
    if digest != ATTACK_HIT_PREFIX_SHA256:
        return 5

    strength_digest = hashlib.sha256(
        program[ATTACK_HIT_STRENGTH_START:ATTACK_HIT_STRENGTH_END]
    ).hexdigest()
    strength_status = (
        "ok" if strength_digest == ATTACK_HIT_STRENGTH_SHA256 else "FAIL"
    )
    print(
        "attack_hit strength corridor "
        f"[0x{ATTACK_HIT_STRENGTH_START:08X},"
        f"0x{ATTACK_HIT_STRENGTH_END:08X}) "
        f"sha256={strength_digest} {strength_status}"
    )
    if strength_digest != ATTACK_HIT_STRENGTH_SHA256:
        return 6

    motion_vector_digest = hashlib.sha256(
        program[ATTACK_HIT_MOTION_VECTOR_START:ATTACK_HIT_MOTION_VECTOR_END]
    ).hexdigest()
    motion_vector_status = (
        "ok"
        if motion_vector_digest == ATTACK_HIT_MOTION_VECTOR_SHA256
        else "FAIL"
    )
    print(
        "attack_hit motion-vector corridor "
        f"[0x{ATTACK_HIT_MOTION_VECTOR_START:08X},"
        f"0x{ATTACK_HIT_MOTION_VECTOR_END:08X}) "
        f"sha256={motion_vector_digest} {motion_vector_status}"
    )
    if motion_vector_digest != ATTACK_HIT_MOTION_VECTOR_SHA256:
        return 7

    print("sfight reference collision/attack signatures: verified")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("romset", type=Path, help="path to local sfight.zip")
    args = parser.parse_args()
    return validate(args.romset)


if __name__ == "__main__":
    raise SystemExit(main())
