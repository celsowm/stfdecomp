#!/usr/bin/env python3
"""
Infer repeated object fields from Model 2 JSONL memory traces.

Expected records are compatible with the vf2-decomp probe trace shape:
  {"type":"memory","step":12,"kind":"read","address":5310848,"size":4}
  {"type":"step","step":12,"ip_before":90240}

This tool is title-agnostic and intentionally uses neutral field offsets.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path


def parse_int(value):
    return value if isinstance(value, int) else int(str(value), 0)


def parse_base(text: str):
    if "=" not in text:
        raise argparse.ArgumentTypeError("base must be NAME=ADDRESS")
    name, raw = text.split("=", 1)
    if not name:
        raise argparse.ArgumentTypeError("base name must not be empty")
    return name, parse_int(raw)


def new_field():
    return {
        "bases": set(),
        "reads": 0,
        "writes": 0,
        "sizes": collections.Counter(),
        "ips": collections.Counter(),
        "addresses": collections.Counter(),
    }


def apply_access(fields, bases, window, access, ip):
    matched = False
    for base_name, base in bases.items():
        offset = access["address"] - base
        if 0 <= offset < window:
            matched = True
            field = fields[offset]
            field["bases"].add(base_name)
            field["sizes"][access["size"]] += 1
            field["addresses"][access["address"]] += 1
            if access["kind"] == "write":
                field["writes"] += 1
            else:
                field["reads"] += 1
            if ip is not None:
                field["ips"][ip] += 1
    return matched


def summarize_trace(path: Path, bases, window):
    fields = collections.defaultdict(new_field)
    pending = collections.defaultdict(list)
    total = 0
    unmatched = 0

    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line.strip():
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            continue
        kind = record.get("type")
        if kind == "memory":
            total += 1
            pending[int(record.get("step", -1))].append(
                {
                    "kind": str(record.get("kind", "read")),
                    "address": parse_int(record.get("address", 0)),
                    "size": int(record.get("size", 0)),
                }
            )
        elif kind == "step":
            step = int(record.get("step", -1))
            ip = parse_int(record.get("ip_before", 0))
            for access in pending.pop(step, []):
                if not apply_access(fields, bases, window, access, ip):
                    unmatched += 1

    for accesses in pending.values():
        for access in accesses:
            if not apply_access(fields, bases, window, access, None):
                unmatched += 1
    return fields, total, unmatched


def records_from(fields):
    records = []
    for offset, data in fields.items():
        records.append(
            {
                "offset": offset,
                "offset_hex": f"+0x{offset:04X}",
                "bases": sorted(data["bases"]),
                "base_count": len(data["bases"]),
                "reads": data["reads"],
                "writes": data["writes"],
                "total": data["reads"] + data["writes"],
                "sizes": dict(sorted(data["sizes"].items())),
                "ips": [
                    {"ip": ip, "count": count}
                    for ip, count in data["ips"].most_common()
                ],
            }
        )
    records.sort(key=lambda item: (-item["base_count"], -item["total"], item["offset"]))
    return records


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", type=Path)
    parser.add_argument("--base", action="append", default=[], type=parse_base)
    parser.add_argument("--window", type=lambda value: int(value, 0), default=0x2000)
    parser.add_argument("--min-count", type=int, default=1)
    parser.add_argument("--limit", type=int, default=100)
    parser.add_argument("--json", dest="json_output", type=Path)
    args = parser.parse_args()

    if not args.base:
        parser.error("provide at least one --base NAME=ADDRESS")
    if args.window < 1 or args.min_count < 1 or args.limit < 0:
        parser.error("window/min-count must be positive and limit non-negative")

    bases = dict(args.base)
    fields, total, unmatched = summarize_trace(args.trace, bases, args.window)
    records = records_from(fields)

    print(f"trace accesses: {total}")
    print("bases: " + ", ".join(f"{name}=0x{value:08X}" for name, value in bases.items()))
    print(f"unmatched accesses: {unmatched}")
    print()

    shown = 0
    for record in records:
        if record["total"] < args.min_count:
            continue
        if args.limit and shown >= args.limit:
            break
        widths = ",".join(f"{size}B x{count}" for size, count in record["sizes"].items())
        ips = ", ".join(
            f"0x{item['ip']:08X} x{item['count']}" for item in record["ips"][:6]
        )
        print(
            f"{record['offset_hex']} bases={','.join(record['bases'])} "
            f"R={record['reads']} W={record['writes']} sizes={widths}"
        )
        if ips:
            print(f"  ips: {ips}")
        shown += 1

    if args.json_output:
        payload = {
            "trace": str(args.trace),
            "bases": bases,
            "window": args.window,
            "accesses": total,
            "unmatched": unmatched,
            "fields": records,
        }
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
