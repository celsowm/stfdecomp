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

HIT_MOTION_SELECTOR_START = 0x0002B94C
HIT_MOTION_SELECTOR_END = 0x0002BA68
HIT_MOTION_SELECTOR_SHA256 = (
    "e6d41cfb273af74a309f3331d783f0eff74e5a908de2c57d2f5c801ec29ac061"
)

DAMAGE_CALCULATION_START = 0x000196DC
DAMAGE_CALCULATION_END = 0x000197C0
DAMAGE_CALCULATION_SHA256 = (
    "c768ac5a1f08f886f37dfb40a3b05e45456ee5429b91bd984846c19e7631b880"
)

ATTACK_HIT_ABORT_PRECHECK_START = 0x0002AC74
ATTACK_HIT_ABORT_PRECHECK_END = 0x0002AC84
ATTACK_HIT_ABORT_PRECHECK_SHA256 = (
    "410dae3ae17a9623485e4b5882928338fd86f7a0cab745b2e66ae88df76da499"
)

ATTACK_HIT_SIDE_EXIT_START = 0x0002AE40
ATTACK_HIT_SIDE_EXIT_END = 0x0002AFB8
ATTACK_HIT_SIDE_EXIT_SHA256 = (
    "9bc7f589a1b3a2ca582f7c97cc405585f98bef222fb611417902a2c7676e111d"
)

ATTACK_HIT_ABORT_CLEANUP_START = 0x0002B8C8
ATTACK_HIT_ABORT_CLEANUP_END = 0x0002B900
ATTACK_HIT_ABORT_CLEANUP_SHA256 = (
    "49e3c6bda5ae14a5d2e52a58923244ee60a5ed7d737e0917b4e525fa80450dd9"
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

    hit_motion_selector_digest = hashlib.sha256(
        program[HIT_MOTION_SELECTOR_START:HIT_MOTION_SELECTOR_END]
    ).hexdigest()
    hit_motion_selector_status = (
        "ok"
        if hit_motion_selector_digest == HIT_MOTION_SELECTOR_SHA256
        else "FAIL"
    )
    print(
        "hit-motion selector corridor "
        f"[0x{HIT_MOTION_SELECTOR_START:08X},"
        f"0x{HIT_MOTION_SELECTOR_END:08X}) "
        f"sha256={hit_motion_selector_digest} {hit_motion_selector_status}"
    )
    if hit_motion_selector_digest != HIT_MOTION_SELECTOR_SHA256:
        return 8

    damage_calculation_digest = hashlib.sha256(
        program[DAMAGE_CALCULATION_START:DAMAGE_CALCULATION_END]
    ).hexdigest()
    damage_calculation_status = (
        "ok"
        if damage_calculation_digest == DAMAGE_CALCULATION_SHA256
        else "FAIL"
    )
    print(
        "damage-calculation corridor "
        f"[0x{DAMAGE_CALCULATION_START:08X},"
        f"0x{DAMAGE_CALCULATION_END:08X}) "
        f"sha256={damage_calculation_digest} {damage_calculation_status}"
    )
    if damage_calculation_digest != DAMAGE_CALCULATION_SHA256:
        return 9

    abort_precheck_digest = hashlib.sha256(
        program[ATTACK_HIT_ABORT_PRECHECK_START:ATTACK_HIT_ABORT_PRECHECK_END]
    ).hexdigest()
    print(
        "attack-hit abort precheck "
        f"sha256={abort_precheck_digest} "
        f"{\"ok\" if abort_precheck_digest == ATTACK_HIT_ABORT_PRECHECK_SHA256 else \"FAIL\"}"
    )
    if abort_precheck_digest != ATTACK_HIT_ABORT_PRECHECK_SHA256:
        return 10

    side_exit_digest = hashlib.sha256(
        program[ATTACK_HIT_SIDE_EXIT_START:ATTACK_HIT_SIDE_EXIT_END]
    ).hexdigest()
    print(
        "attack-hit side-exit corridor "
        f"sha256={side_exit_digest} "
        f"{\"ok\" if side_exit_digest == ATTACK_HIT_SIDE_EXIT_SHA256 else \"FAIL\"}"
    )
    if side_exit_digest != ATTACK_HIT_SIDE_EXIT_SHA256:
        return 11

    abort_cleanup_digest = hashlib.sha256(
        program[ATTACK_HIT_ABORT_CLEANUP_START:ATTACK_HIT_ABORT_CLEANUP_END]
    ).hexdigest()
    print(
        "attack-hit abort cleanup "
        f"sha256={abort_cleanup_digest} "
        f"{\"ok\" if abort_cleanup_digest == ATTACK_HIT_ABORT_CLEANUP_SHA256 else \"FAIL\"}"
    )
    if abort_cleanup_digest != ATTACK_HIT_ABORT_CLEANUP_SHA256:
        return 12

    print("sfight reference collision/attack signatures: verified")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("romset", type=Path, help="path to local sfight.zip")
    args = parser.parse_args()
    return validate(args.romset)


if __name__ == "__main__":
    raise SystemExit(main())
