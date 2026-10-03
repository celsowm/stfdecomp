#!/usr/bin/env python3
"""
Aggregate first-fault observations from STF recovery JSONL traces.

The tool reports evidence only. It does not infer device semantics.
Newer corridor traces include the failing step, i960 IP/instruction and
write payload; older traces remain accepted with those fields empty.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path


def fault_from_record(path: Path, line_number: int, record: dict):
    if record.get("type") == "fault":
        source = record
        fallback = record
    elif record.get("type") == "halt" and isinstance(record.get("bus_fault"), dict):
        source = record["bus_fault"]
        fallback = record
    else:
        return None

    return {
        "trace": str(path),
        "line": line_number,
        "kind": source.get("kind", "unknown"),
        "address": int(source.get("address", 0)),
        "size": int(source.get("size", 0)),
        "status": source.get("status", fallback.get("status_text", "")),
        "region": source.get("region", ""),
        "symbol": source.get("symbol", ""),
        "step": int(source.get("step", fallback.get("step", 0))),
        "ip": int(source.get("ip", fallback.get("ip", 0))),
        "instruction": source.get("instruction", ""),
        "bytes": source.get("bytes", ""),
    }


def load_faults(path: Path):
    faults = []
    for line_number, line in enumerate(
        path.read_text(encoding="utf-8", errors="replace").splitlines(),
        1,
    ):
        if not line.startswith("{"):
            continue
        try:
            record = json.loads(line)
        except json.JSONDecodeError:
            continue

        fault = fault_from_record(path, line_number, record)
        if fault is not None:
            faults.append(fault)

    return faults


def most_common(counter: collections.Counter, default=""):
    return counter.most_common(1)[0][0] if counter else default


def aggregate(paths: list[Path]):
    groups = {}

    for path in paths:
        for fault in load_faults(path):
            key = (fault["kind"], fault["address"], fault["size"])
            item = groups.setdefault(
                key,
                {
                    "kind": fault["kind"],
                    "address": fault["address"],
                    "address_hex": f"0x{fault['address']:08X}",
                    "size": fault["size"],
                    "count": 0,
                    "traces": set(),
                    "regions": collections.Counter(),
                    "symbols": collections.Counter(),
                    "statuses": collections.Counter(),
                    "ips": collections.Counter(),
                    "instructions": collections.Counter(),
                    "payloads": collections.Counter(),
                    "first": {
                        "trace": fault["trace"],
                        "line": fault["line"],
                        "step": fault["step"],
                        "ip": fault["ip"],
                        "instruction": fault["instruction"],
                        "bytes": fault["bytes"],
                    },
                },
            )

            item["count"] += 1
            item["traces"].add(fault["trace"])
            if fault["region"]:
                item["regions"][fault["region"]] += 1
            if fault["symbol"]:
                item["symbols"][fault["symbol"]] += 1
            if fault["status"] != "":
                item["statuses"][str(fault["status"])] += 1
            if fault["ip"] != 0:
                item["ips"][fault["ip"]] += 1
            if fault["instruction"]:
                item["instructions"][fault["instruction"]] += 1
            if fault["bytes"]:
                item["payloads"][fault["bytes"]] += 1

    rows = []
    for item in groups.values():
        ip = most_common(item["ips"], 0)
        rows.append(
            {
                "kind": item["kind"],
                "address": item["address"],
                "address_hex": item["address_hex"],
                "size": item["size"],
                "count": item["count"],
                "trace_count": len(item["traces"]),
                "region": most_common(item["regions"]),
                "symbol": most_common(item["symbols"]),
                "statuses": dict(item["statuses"]),
                "ip": ip,
                "ip_hex": f"0x{ip:08X}" if ip else "",
                "instruction": most_common(item["instructions"]),
                "bytes": most_common(item["payloads"]),
                "first": item["first"],
                "traces": sorted(item["traces"]),
            }
        )

    rows.sort(
        key=lambda row: (
            -row["trace_count"],
            -row["count"],
            row["address"],
            row["ip"],
        )
    )
    return rows


def render_markdown(rows) -> str:
    lines = [
        "# STF recovery fault summary",
        "",
        "| Traces | Hits | Kind | Address | Size | IP | Bytes | Region | Symbol | Instruction | First trace |",
        "| ---: | ---: | --- | --- | ---: | --- | --- | --- | --- | --- | --- |",
    ]

    for row in rows:
        first = f"{row['first']['trace']}:{row['first']['line']}"
        lines.append(
            f"| {row['trace_count']} | {row['count']} | {row['kind']} | "
            f"{row['address_hex']} | {row['size']} | {row['ip_hex'] or '-'} | "
            f"{row['bytes'] or '-'} | {row['region'] or '-'} | "
            f"{row['symbol'] or '-'} | {row['instruction'] or '-'} | {first} |"
        )

    if not rows:
        lines.append("| - | - | - | - | - | - | - | - | - | - | no faults found |")

    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", nargs="+", type=Path)
    parser.add_argument("--json", dest="json_output", type=Path)
    parser.add_argument("--markdown", dest="markdown_output", type=Path)
    args = parser.parse_args()

    rows = aggregate(args.trace)

    for row in rows:
        print(
            f"{row['trace_count']:>3} traces  {row['count']:>4} hits  "
            f"{row['kind']:<5} {row['address_hex']} size={row['size']} "
            f"ip={row['ip_hex'] or '-'} bytes={row['bytes'] or '-'} "
            f"region={row['region'] or '-'} symbol={row['symbol'] or '-'} "
            f"instruction={row['instruction'] or '-'}"
        )

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(
            json.dumps({"faults": rows}, indent=2) + "\n",
            encoding="utf-8",
        )

    if args.markdown_output:
        args.markdown_output.parent.mkdir(parents=True, exist_ok=True)
        args.markdown_output.write_text(
            render_markdown(rows),
            encoding="utf-8",
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
