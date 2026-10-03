#!/usr/bin/env python3
"""
Classify Model 2 TGP/geometry traffic from JSONL memory traces.

The defaults reflect the common Model 2 geometry/TGP windows used by the
current VF2 research. All addresses are configurable so Model 2B differences
can be measured rather than assumed.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path


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


def classify(path, fifo_address, geo_start, geo_end, function_ports):
    fifo = []
    geometry = []
    classes = collections.Counter()
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

        if address == fifo_address:
            fifo.append(value)
            classes[(value >> 23) & 0x1F] += 1
        elif geo_start <= address < geo_end:
            geometry.append((address, value))
        elif address in function_ports:
            port_writes[address] += 1

    return {
        "trace": str(path),
        "fifo_writes": len(fifo),
        "geometry_writes": len(geometry),
        "function_port_writes": {
            f"0x{address:08X}": count for address, count in sorted(port_writes.items())
        },
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
    parser.add_argument("--fifo", type=parse_int, default=0x00884000)
    parser.add_argument("--geometry-start", type=parse_int, default=0x00800000)
    parser.add_argument("--geometry-end", type=parse_int, default=0x00808000)
    parser.add_argument(
        "--function-port",
        action="append",
        type=parse_int,
        default=[],
        help="repeatable TGP/coprocessor function port address",
    )
    parser.add_argument("--json", dest="json_output", type=Path)
    args = parser.parse_args()

    ports = set(args.function_port or [0x00980000, 0x00880000])
    reports = [
        classify(path, args.fifo, args.geometry_start, args.geometry_end, ports)
        for path in args.trace
    ]

    for report in reports:
        print(
            f"{report['trace']}: fifo={report['fifo_writes']} "
            f"geometry={report['geometry_writes']} "
            f"ports={sum(report['function_port_writes'].values())}"
        )
        print(f"  classes={report['command_classes'][:16]}")
        if report["fifo_sample"]:
            print(f"  fifo sample={report['fifo_sample']}")
        if report["geometry_sample"]:
            print(f"  geometry sample={report['geometry_sample'][:8]}")

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(
            json.dumps({"reports": reports}, indent=2) + "\n",
            encoding="utf-8",
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
