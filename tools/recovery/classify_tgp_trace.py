#!/usr/bin/env python3
"""
Classify Sonic the Fighters Model 2B geometry/coprocessor traffic from JSONL
memory traces.

Defaults come from STF's own src/lib/rom_code1.ld. No VF2-only FIFO address is
assumed. Use --fifo only after a concrete STF trace establishes one.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path


KNOWN_COMMANDS = {
    0x0D001A1A: "scalar_sqrt",
}

STF_DEFAULT_PORTS = [
    0x008C0000,  # COPRO_SHARC_IOP_START
    0x00980000,  # COPRO_CONTROL1_START
    0x00980008,  # GEO_CTL1_START
    0x00980014,  # COPRO_STATUS_START
]


def parse_int(text):
    return int(str(text), 0)


def value_from_record(record):
    if "value" in record:
        return parse_int(record["value"])
    raw = record.get("bytes", "")
    try:
        data = bytes.fromhex(raw)
    except ValueError:
        return 0
    return int.from_bytes(data[:4].ljust(4, b"\x00"), "little")


def classify(path, fifo_address, geo_start, geo_end, watched_ports):
    fifo = []
    geometry = []
    classes = collections.Counter()
    known_commands = collections.Counter()
    port_writes = collections.Counter()

    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line.startswith("{"):
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            continue
        if record.get("type") != "memory" or record.get("kind") != "write":
            continue
        address = parse_int(record.get("address", 0))
        value = value_from_record(record)

        if fifo_address is not None and address == fifo_address:
            fifo.append(value)
            classes[(value >> 23) & 0x1F] += 1
            command_name = KNOWN_COMMANDS.get(value)
            if command_name is not None:
                known_commands[command_name] += 1
        elif geo_start <= address < geo_end:
            geometry.append((address, value))
        elif address in watched_ports:
            port_writes[address] += 1

    return {
        "trace": str(path),
        "fifo_address": (
            f"0x{fifo_address:08X}" if fifo_address is not None else None
        ),
        "fifo_writes": len(fifo),
        "geometry_window": {
            "start": f"0x{geo_start:08X}",
            "end": f"0x{geo_end:08X}",
        },
        "geometry_writes": len(geometry),
        "watched_port_writes": {
            f"0x{address:08X}": count for address, count in sorted(port_writes.items())
        },
        "known_commands": [
            {"command": name, "count": count}
            for name, count in known_commands.most_common()
        ],
        "command_classes": [
            {"class": command_class, "count": count}
            for command_class, count in classes.most_common()
        ],
        "fifo_sample": [f"0x{value:08X}" for value in fifo[:16]],
        "geometry_sample": [
            {"address": f"0x{address:08X}", "value": f"0x{value:08X}"}
            for address, value in geometry[:16]
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", nargs="+", type=Path)
    parser.add_argument(
        "--fifo",
        type=parse_int,
        default=None,
        help="STF FIFO address, only when established by trace evidence",
    )
    parser.add_argument("--geometry-start", type=parse_int, default=0x00800000)
    parser.add_argument("--geometry-end", type=parse_int, default=0x00804000)
    parser.add_argument(
        "--function-port",
        action="append",
        type=parse_int,
        default=[],
        help="repeatable watched coprocessor/control address",
    )
    parser.add_argument("--json", dest="json_output", type=Path)
    args = parser.parse_args()

    ports = set(args.function_port or STF_DEFAULT_PORTS)
    reports = [
        classify(
            path,
            args.fifo,
            args.geometry_start,
            args.geometry_end,
            ports,
        )
        for path in args.trace
    ]

    for report in reports:
        fifo_text = (
            str(report["fifo_writes"])
            if report["fifo_address"] is not None
            else "disabled"
        )
        print(
            f"{report['trace']}: fifo={fifo_text} "
            f"geometry={report['geometry_writes']} "
            f"ports={sum(report['watched_port_writes'].values())}"
        )
        if report["known_commands"]:
            print(f"  known={report['known_commands'][:16]}")
        if report["command_classes"]:
            print(f"  classes={report['command_classes'][:16]}")
        if report["fifo_sample"]:
            print(f"  fifo sample={report['fifo_sample']}")
        if report["geometry_sample"]:
            print(f"  geometry sample={report['geometry_sample'][:8]}")
        if report["watched_port_writes"]:
            print(f"  watched ports={report['watched_port_writes']}")

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(
            json.dumps({"reports": reports}, indent=2) + "\n",
            encoding="utf-8",
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
