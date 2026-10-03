#!/usr/bin/env python3
"""
Run a reproducible STF i960 recovery scenario.

A scenario describes the corridor in JSON using symbols where possible. The
runner resolves entry/stop symbols from an nm960/GNU-nm/linker-map listing and
invokes stf_i960_corridor with explicit recovery parameters.

No ROM data is embedded in scenario files.
"""

from __future__ import annotations

import argparse
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

from extract_task_descriptors import extract as extract_task_descriptors
from resolve_symbol import parse_symbols


def parse_int(value) -> int:
    if isinstance(value, int):
        return value
    return int(str(value), 0)


def resolve_location(value, symbols: dict[str, int], field: str) -> int:
    if isinstance(value, int):
        return value
    text = str(value)
    try:
        return int(text, 0)
    except ValueError:
        pass
    if text not in symbols:
        raise SystemExit(f"{field} symbol not found: {text}")
    return symbols[text]


def resolve_task_descriptor(scenario: dict, repo_root: Path):
    task_name = scenario.get("task")
    if not task_name:
        return None

    table = Path(scenario.get("task_table", "src/asm/rom_code2.s"))
    if not table.is_absolute():
        table = repo_root / table

    matches = [
        item
        for item in extract_task_descriptors(table)
        if item["task"] == task_name
    ]
    if not matches:
        raise SystemExit(f"task descriptor not found: {task_name}")
    if len(matches) != 1:
        raise SystemExit(
            f"task descriptor is ambiguous: {task_name} ({len(matches)} matches)"
        )
    return matches[0]


def executable_candidates(repo_root: Path) -> list[Path]:
    return [
        repo_root / "build" / "recovery-i960" / "stf_i960_corridor",
        repo_root / "build" / "recovery-i960" / "Release" / "stf_i960_corridor.exe",
        repo_root / "build" / "recovery-i960" / "stf_i960_corridor.exe",
    ]


def find_runner(repo_root: Path, explicit: str | None) -> Path:
    if explicit:
        candidate = Path(explicit)
        if not candidate.is_absolute():
            candidate = repo_root / candidate
        if candidate.exists():
            return candidate
        raise SystemExit(f"corridor runner not found: {candidate}")

    for candidate in executable_candidates(repo_root):
        if candidate.exists():
            return candidate
    raise SystemExit(
        "stf_i960_corridor not found; build tools/recovery/i960 first"
    )


def optional_repo_file(
    repo_root: Path,
    explicit_value,
    default_relative: str,
    field: str,
) -> Path | None:
    value = explicit_value if explicit_value is not None else default_relative
    path = Path(value)
    if not path.is_absolute():
        path = repo_root / path

    if explicit_value is not None and not path.exists():
        raise SystemExit(f"{field} file not found: {path}")
    return path if path.exists() else None


def output_path(
    repo_root: Path,
    scenario_path: Path,
    scenario: dict,
    key: str,
    default_suffix: str,
) -> Path:
    value = scenario.get(key)
    if value:
        path = Path(value)
        return path if path.is_absolute() else repo_root / path

    name = scenario.get("name") or scenario_path.stem
    return repo_root / "out" / "recovery" / f"{name}{default_suffix}"


def add_registers(command: list[str], registers: dict) -> None:
    for name, value in registers.items():
        command.extend(["--reg", f"{name}={parse_int(value):#x}"])


def add_probe_policy(command: list[str], policy: dict) -> None:
    for item in policy.get("allow_write", []):
        if isinstance(item, str):
            value = item
        else:
            start = parse_int(item["start"])
            end = parse_int(item["end"])
            value = f"{start:#x}:{end:#x}"
        command.extend(["--allow-write", value])

    stubs = policy.get("stub_read", {})
    if isinstance(stubs, list):
        for item in stubs:
            command.extend(
                [
                    "--stub-read",
                    f"{parse_int(item['address']):#x}={parse_int(item['value']):#x}",
                ]
            )
    else:
        for address, value in stubs.items():
            command.extend(
                ["--stub-read", f"{parse_int(address):#x}={parse_int(value):#x}"]
            )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("scenario", type=Path)
    parser.add_argument(
        "--symbols",
        type=Path,
        help="nm960/GNU-nm/linker map; overrides scenario symbols",
    )
    parser.add_argument("--runner", help="path to stf_i960_corridor")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[2],
    )
    args = parser.parse_args()

    repo_root = args.repo_root.resolve()
    scenario_path = args.scenario.resolve()
    scenario = json.loads(scenario_path.read_text(encoding="utf-8"))

    if scenario.get("template", False):
        raise SystemExit(
            "scenario is marked template=true; copy it and record measured inputs first"
        )

    symbol_path_value = args.symbols or scenario.get("symbols")
    symbols: dict[str, int] = {}
    if symbol_path_value:
        symbol_path = Path(symbol_path_value)
        if not symbol_path.is_absolute():
            symbol_path = repo_root / symbol_path
        symbols = parse_symbols(symbol_path)
        if not symbols:
            raise SystemExit(f"no symbols recognized in {symbol_path}")

    task_descriptor = resolve_task_descriptor(scenario, repo_root)
    entry_value = scenario.get("entry")
    if entry_value is None and task_descriptor is not None:
        entry_value = task_descriptor["init"]
    if entry_value is None:
        raise SystemExit("scenario requires entry or task")
    entry = resolve_location(entry_value, symbols, "entry")

    runner = find_runner(repo_root, args.runner)
    rom = Path(scenario.get("rom", "rom/rom_code1.bin"))
    if not rom.is_absolute():
        rom = repo_root / rom
    if not rom.exists():
        raise SystemExit(
            f"program ROM not found: {rom}; "
            "extract local ROM data with: python tools/data_extract.py --rom"
        )

    data_rom = optional_repo_file(
        repo_root,
        scenario.get("data_rom"),
        "rom/rom_data.bin",
        "data_rom",
    )
    ep_rom = optional_repo_file(
        repo_root,
        scenario.get("ep_rom"),
        "rom/rom_ep.bin",
        "ep_rom",
    )

    trace = output_path(repo_root, scenario_path, scenario, "trace", ".jsonl")
    state = output_path(repo_root, scenario_path, scenario, "state", "-state.json")
    trace.parent.mkdir(parents=True, exist_ok=True)
    state.parent.mkdir(parents=True, exist_ok=True)

    command = [
        str(runner),
        "--rom",
        str(rom),
        "--entry",
        f"{entry:#x}",
        "--steps",
        str(parse_int(scenario.get("steps", 100000))),
        "--trace",
        str(trace),
        "--state",
        str(state),
    ]

    if data_rom is not None:
        command.extend(["--data-rom", str(data_rom)])
    if ep_rom is not None:
        command.extend(["--ep-rom", str(ep_rom)])

    if "stop" in scenario and scenario["stop"] is not None:
        stop = resolve_location(scenario["stop"], symbols, "stop")
        command.extend(["--stop", f"{stop:#x}"])
    if "stack" in scenario:
        command.extend(["--stack", f"{parse_int(scenario['stack']):#x}"])
    if "sat" in scenario:
        command.extend(["--sat", f"{parse_int(scenario['sat']):#x}"])
    if "prcb" in scenario:
        command.extend(["--prcb", f"{parse_int(scenario['prcb']):#x}"])
    if scenario.get("reset_from_prcb", False):
        command.append("--reset-from-prcb")

    add_registers(command, scenario.get("registers", {}))

    work_ram_in = scenario.get("work_ram_in")
    if work_ram_in:
        path = Path(work_ram_in)
        if not path.is_absolute():
            path = repo_root / path
        command.extend(["--work-ram-in", str(path)])

    work_ram_out = scenario.get("work_ram_out")
    if work_ram_out:
        path = Path(work_ram_out)
        if not path.is_absolute():
            path = repo_root / path
        path.parent.mkdir(parents=True, exist_ok=True)
        command.extend(["--work-ram-out", str(path)])

    probe = scenario.get("probe", {})
    add_probe_policy(command, probe)

    print("scenario:", scenario.get("name", scenario_path.stem))
    if task_descriptor is not None:
        print(
            "task:",
            task_descriptor["task"],
            "workspace=" + task_descriptor["workspace_size_hex"],
            "init=" + task_descriptor["init"],
            "update=" + task_descriptor["update"],
        )
        print(
            "task-context: descriptor validated; runtime task-instance state is not synthesized"
        )
    print("entry:", f"0x{entry:08X}", entry_value)
    print("program-rom:", rom)
    print("data-rom:", data_rom if data_rom is not None else "not attached")
    print("ep-rom:", ep_rom if ep_rom is not None else "not attached")
    print("trace:", trace)
    print("state:", state)
    if probe:
        print("mode: exploratory device policy")
    else:
        print("mode: fail-closed reference candidate")
    print("command:", shlex.join(command))

    if args.dry_run:
        return 0

    completed = subprocess.run(command, cwd=repo_root, check=False)
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
