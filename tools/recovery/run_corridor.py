#!/usr/bin/env python3
"""
Resolve STF symbols and launch the host i960 corridor runner.

Examples:

    nm960 -n temp/rom_code1.out > build/rom_code1.nm

    python tools/recovery/run_corridor.py \
        --symbols build/rom_code1.nm \
        --entry set_obj \
        --rom rom/rom_code1.bin \
        --trace out/set_obj.jsonl \
        --state out/set_obj-state.json \
        --stack 0x005ff600 \
        --reg g0=0

Unknown options are forwarded to stf_i960_corridor.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import subprocess
import sys

from resolve_symbol import parse_symbols


def parse_numeric(text: str) -> int | None:
    try:
        return int(text, 0)
    except ValueError:
        return None


def resolve_value(text: str, symbols: dict[str, int]) -> str:
    numeric = parse_numeric(text)
    if numeric is not None:
        if numeric < 0 or numeric > 0xFFFFFFFF:
            raise ValueError(f"address out of range: {text}")
        return f"0x{numeric:08X}"

    if text not in symbols:
        raise ValueError(f"symbol not found: {text}")
    return f"0x{symbols[text]:08X}"


def runner_candidates(repo_root: Path) -> list[Path]:
    exe = "stf_i960_corridor.exe" if os.name == "nt" else "stf_i960_corridor"
    build = repo_root / "build" / "recovery-i960"
    return [
        build / exe,
        build / "Release" / exe,
        build / "Debug" / exe,
        build / "RelWithDebInfo" / exe,
    ]


def locate_runner(repo_root: Path, explicit: Path | None) -> Path:
    if explicit is not None:
        return explicit
    for candidate in runner_candidates(repo_root):
        if candidate.exists():
            return candidate
    joined = "\n  ".join(str(path) for path in runner_candidates(repo_root))
    raise FileNotFoundError(
        "corridor runner not found; build it first with:\n"
        "  cmake -S tools/recovery/i960 -B build/recovery-i960 "
        "-DCMAKE_BUILD_TYPE=Release\n"
        "  cmake --build build/recovery-i960 --config Release\n"
        f"looked in:\n  {joined}"
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path)
    parser.add_argument("--rom", type=Path, default=Path("rom/rom_code1.bin"))
    parser.add_argument("--symbols", type=Path)
    parser.add_argument("--entry", default="0")
    parser.add_argument("--stop")
    args, forwarded = parser.parse_known_args()

    repo_root = Path(__file__).resolve().parents[2]
    symbols: dict[str, int] = {}
    if args.symbols is not None:
        symbols = parse_symbols(args.symbols)

    try:
        entry = resolve_value(args.entry, symbols)
        stop = resolve_value(args.stop, symbols) if args.stop is not None else None
        runner = locate_runner(repo_root, args.runner)
    except (ValueError, FileNotFoundError) as exc:
        parser.error(str(exc))

    rom = args.rom
    if not rom.is_absolute():
        rom = repo_root / rom
    if not rom.exists():
        parser.error(
            f"ROM image not found: {rom}\n"
            "extract your local original ROM set with: "
            "python tools/data_extract.py --rom"
        )

    command = [
        str(runner),
        "--rom",
        str(rom),
        "--entry",
        entry,
    ]
    if stop is not None:
        command.extend(["--stop", stop])
    command.extend(forwarded)

    print("+", subprocess.list2cmdline(command))
    completed = subprocess.run(command, cwd=repo_root, check=False)
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
