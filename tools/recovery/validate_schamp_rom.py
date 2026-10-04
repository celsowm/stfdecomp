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
PROGRAM_CRCS = {
    "epr-19141.15": 0xB942EF21,
    "epr-19142.16": 0x2D54BD76,
}
PROGRAM_SHA256 = (
    "cc7294a03a486b11ee035791a889584223c33e478cf31ed14a60e33019dd7fcd"
)

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

ATTACK_HIT_PREFIX_START = 0x0002A858
ATTACK_HIT_PREFIX_END = 0x0002A8F8
ATTACK_HIT_PREFIX_SHA256 = (
    "75468d8e674e8b6b8c6334cdb1512f6f69fe2067c3ca2d9ac05a354c55ab671a"
)

ATTACK_HIT_KAMAE_GATE_START = 0x0002AE80
ATTACK_HIT_KAMAE_GATE_END = 0x0002AF24
ATTACK_HIT_KAMAE_GATE_SHA256 = (
    "b6df86ba6e0452c65d0b28c3eb2ca70c50596c1ad126e04357287ff90b39457f"
)

SET_KAMAE_START = 0x0002F274
SET_KAMAE_END = 0x0002F2DC
SET_KAMAE_SHA256 = (
    "41020fa4dfa07e52157e41511cd0c6fd6577a2658b4684114ff7b5af108ac222"
)

GET_KAMAE_START = 0x0002FDB0
GET_KAMAE_END = 0x0002FF34
GET_KAMAE_SHA256 = (
    "3e426637f59d11beb7e7616687d2346de8618cd4d9ba854bd0788ad373aa2daa"
)

# Sonic Championship keeps the same ring-scatter data layout as sfight but the
# secondary program shifts these tables by +0x138.
RING_RENDER_TABLE_START = 0x000AE44C
RING_RENDER_TABLE_END = 0x000AE4E0
RING_RENDER_TABLE_SHA256 = (
    "60b9ccf507505c334b13b93e8d27f724974271b7e73d30b36ec42c57c02859c2"
)
RING_PRIMARY_ASSETS = (
    0x2C3, 0x2C4, 0x2C5, 0x2C6, 0x2C7, 0x2C8, 0x2C9, 0x2CA,
    0x2CB, 0x2CC, 0x2CD, 0x2CE, 0x2CF, 0x2D0, 0x2D1, 0xD9B,
)
RING_DROP_ASSETS = (0x7BE, 0x7BF, 0x7C0, 0x7BE, 0x6AC)
RING_STAGE2_ASSETS = (
    0x332, 0x333, 0x334, 0x335, 0x336, 0x337, 0x338, 0x339,
    0x33A, 0x35B, 0x35C, 0x3AA, 0x3AB, 0x3AC, 0x3AD, 0xD9C,
)

RING_VECTOR_START = 0x000AE4E0
RING_VECTOR_END = 0x000AE560
RING_VECTOR_SHA256 = (
    "e9be122322393d39f1eaf69189797b43bb67c9d31578b572fa639cac9d5460f4"
)

RING_PROFILE_START = 0x000AE560
RING_PROFILE_END = 0x000AE5C0
RING_PROFILE_SHA256 = (
    "27a66a49e2968878d069dfda5fa81b9be1fd500f1450a32cb7079e77e876a534"
)

RING_RUNTIME_START = 0x00078E84
RING_RUNTIME_END = 0x000791F4
RING_RUNTIME_SHA256 = (
    "2acb82139cd59c3adb3456242abb3425912eaafa31f909fb7a1bf59bdbf62460"
)

RING_POOL_HELPERS_START = 0x000791F8
RING_POOL_HELPERS_END = 0x00079270
RING_POOL_HELPERS_SHA256 = (
    "2a6956dc83fdb627fdc7e9a0a737273f386f1fae1553baae8abdbdb471183779"
)

RING_TRAJECTORIES = {
    "A": (0x000AE5C0, 99),
    "B": (0x000AE750, 119),
    "C": (0x000AE930, 139),
}

# Eight threshold/scale/trajectory triples consumed by ring_tobitiri_set.
RING_PROFILE_WORDS = (
    0x00000002, 0x3F000000, 0x000AE5C0,
    0x00000008, 0x3F4CCCCD, 0x000AE750,
    0x00000008, 0x3F4CCCCD, 0x000AE930,
    0x00000010, 0x3F99999A, 0x000AE750,
    0x00000008, 0x3F800000, 0x000AE930,
    0x00000010, 0x3FB33333, 0x000AE750,
    0x00000018, 0x3FE66666, 0x000AE5C0,
    0x00000008, 0x3FCCCCCD, 0x000AE930,
)


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

        print("program CRCs:")
        for name, expected in PROGRAM_CRCS.items():
            actual = archive.getinfo(name).CRC
            status = "ok" if actual == expected else "FAIL"
            print(f"  {name}: {actual:08x} {status}")
            if actual != expected:
                return 4

        program = interleave_words(
            archive.read(PROGRAM_PAIR[0]),
            archive.read(PROGRAM_PAIR[1]),
        )

    print(f"program size: 0x{len(program):X}")
    program_digest = hashlib.sha256(program).hexdigest()
    program_status = "ok" if program_digest == PROGRAM_SHA256 else "FAIL"
    print(f"program sha256: {program_digest} {program_status}")
    if program_digest != PROGRAM_SHA256:
        return 5
    print(f"coli_init: 0x{COLI_INIT:08X}")
    print(f"collision: 0x{COLLISION:08X}")

    for offset, expected in COLI_CONSTANTS.items():
        if offset + 4 > len(program):
            print(f"signature offset outside program: 0x{offset:08X}")
            return 6
        actual = struct.unpack_from("<I", program, offset)[0]
        status = "ok" if actual == expected else "FAIL"
        print(
            f"  [0x{offset:08X}] 0x{actual:08X} "
            f"expected=0x{expected:08X} {status}"
        )
        if actual != expected:
            return 7

    attack_hit_prefix = program[ATTACK_HIT_PREFIX_START:ATTACK_HIT_PREFIX_END]
    attack_hit_digest = hashlib.sha256(attack_hit_prefix).hexdigest()
    attack_status = (
        "ok" if attack_hit_digest == ATTACK_HIT_PREFIX_SHA256 else "FAIL"
    )
    print(
        "attack_hit prefix "
        f"[0x{ATTACK_HIT_PREFIX_START:08X},0x{ATTACK_HIT_PREFIX_END:08X}) "
        f"sha256={attack_hit_digest} {attack_status}"
    )
    if attack_hit_digest != ATTACK_HIT_PREFIX_SHA256:
        return 8


    for label, start, end, expected_digest in (
        (
            "attack_hit set_kamae gate",
            ATTACK_HIT_KAMAE_GATE_START,
            ATTACK_HIT_KAMAE_GATE_END,
            ATTACK_HIT_KAMAE_GATE_SHA256,
        ),
        ("set_kamae_ram", SET_KAMAE_START, SET_KAMAE_END, SET_KAMAE_SHA256),
        ("get_kamae_value", GET_KAMAE_START, GET_KAMAE_END, GET_KAMAE_SHA256),
    ):
        digest = hashlib.sha256(program[start:end]).hexdigest()
        status = "ok" if digest == expected_digest else "FAIL"
        print(
            f"{label} [0x{start:08X},0x{end:08X}) "
            f"sha256={digest} {status}"
        )
        if digest != expected_digest:
            return 16

    ring_render_tables = program[
        RING_RENDER_TABLE_START:RING_RENDER_TABLE_END
    ]
    ring_render_digest = hashlib.sha256(ring_render_tables).hexdigest()
    ring_render_status = (
        "ok" if ring_render_digest == RING_RENDER_TABLE_SHA256 else "FAIL"
    )
    print(
        "ring render/drop tables "
        f"[0x{RING_RENDER_TABLE_START:08X},0x{RING_RENDER_TABLE_END:08X}) "
        f"sha256={ring_render_digest} {ring_render_status}"
    )
    if ring_render_digest != RING_RENDER_TABLE_SHA256:
        return 9

    render_words = struct.unpack(
        "<" + "I" * (
            len(RING_PRIMARY_ASSETS)
            + len(RING_DROP_ASSETS)
            + len(RING_STAGE2_ASSETS)
        ),
        ring_render_tables,
    )
    expected_render_words = (
        RING_PRIMARY_ASSETS + RING_DROP_ASSETS + RING_STAGE2_ASSETS
    )
    if render_words != expected_render_words:
        print("ring render/drop asset IDs: FAIL")
        return 10
    print("ring render/drop asset IDs: ok")

    ring_vectors = program[RING_VECTOR_START:RING_VECTOR_END]
    ring_vector_digest = hashlib.sha256(ring_vectors).hexdigest()
    ring_vector_status = (
        "ok" if ring_vector_digest == RING_VECTOR_SHA256 else "FAIL"
    )
    print(
        "ring scatter local vectors "
        f"[0x{RING_VECTOR_START:08X},0x{RING_VECTOR_END:08X}) "
        f"sha256={ring_vector_digest} {ring_vector_status}"
    )
    if ring_vector_digest != RING_VECTOR_SHA256:
        return 9

    ring_profiles = program[RING_PROFILE_START:RING_PROFILE_END]
    ring_profile_digest = hashlib.sha256(ring_profiles).hexdigest()
    ring_profile_status = (
        "ok" if ring_profile_digest == RING_PROFILE_SHA256 else "FAIL"
    )
    print(
        "ring scatter profile records "
        f"[0x{RING_PROFILE_START:08X},0x{RING_PROFILE_END:08X}) "
        f"sha256={ring_profile_digest} {ring_profile_status}"
    )
    if ring_profile_digest != RING_PROFILE_SHA256:
        return 10

    profile_words = struct.unpack(
        "<" + "I" * len(RING_PROFILE_WORDS),
        ring_profiles,
    )
    if profile_words != RING_PROFILE_WORDS:
        print("ring scatter profile triples: FAIL")
        return 11
    print("ring scatter profile triples: ok")

    for name, (address, sample_count) in RING_TRAJECTORIES.items():
        sentinel_offset = address + sample_count * 4
        if sentinel_offset + 4 > len(program):
            print(f"ring trajectory {name}: sentinel outside program")
            return 12
        sentinel = struct.unpack_from("<I", program, sentinel_offset)[0]
        status = "ok" if sentinel == 0xBF800000 else "FAIL"
        print(
            f"ring trajectory {name}: samples={sample_count} "
            f"sentinel@0x{sentinel_offset:08X}=0x{sentinel:08X} {status}"
        )
        if sentinel != 0xBF800000:
            return 13

    ring_runtime = program[RING_RUNTIME_START:RING_RUNTIME_END]
    ring_runtime_digest = hashlib.sha256(ring_runtime).hexdigest()
    ring_runtime_status = (
        "ok" if ring_runtime_digest == RING_RUNTIME_SHA256 else "FAIL"
    )
    print(
        "ring per-frame runtime "
        f"[0x{RING_RUNTIME_START:08X},0x{RING_RUNTIME_END:08X}) "
        f"sha256={ring_runtime_digest} {ring_runtime_status}"
    )
    if ring_runtime_digest != RING_RUNTIME_SHA256:
        return 14

    ring_pool_helpers = program[
        RING_POOL_HELPERS_START:RING_POOL_HELPERS_END
    ]
    ring_pool_helpers_digest = hashlib.sha256(ring_pool_helpers).hexdigest()
    ring_pool_helpers_status = (
        "ok"
        if ring_pool_helpers_digest == RING_POOL_HELPERS_SHA256
        else "FAIL"
    )
    print(
        "ring pool bit helpers "
        f"[0x{RING_POOL_HELPERS_START:08X},0x{RING_POOL_HELPERS_END:08X}) "
        f"sha256={ring_pool_helpers_digest} {ring_pool_helpers_status}"
    )
    if ring_pool_helpers_digest != RING_POOL_HELPERS_SHA256:
        return 15

    print("schamp collision/ring signatures: verified (secondary evidence)")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("romset", type=Path, help="path to local schamp.zip")
    args = parser.parse_args()
    return validate(args.romset)


if __name__ == "__main__":
    raise SystemExit(main())
