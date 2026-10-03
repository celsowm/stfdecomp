#!/usr/bin/env python3
"""
Aggregate first-fault observations from STF recovery JSONL traces.

The tool intentionally reports evidence; it does not infer device semantics.
If corridor traces contain linker-derived region/symbol hints, they are carried
into the report.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path


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

        if record.get("type") == "fault":
            faults.append(
                {
                    "trace": str(path),
                    "line": line_number,
                    "kind": record.get("kind", "unknown"),
                    "address": int(record.get("address", 0)),
                    "size": int(record.get("size", 0)),
                    "status": record.get("status", ""),
                    "region": record.get("region", ""),
                    "symbol": record.get("symbol", ""),
                }
            )
            continue

        if record.get("type") == "halt" and isinstance(record.get("bus_fault"), dict):
            fault = record["bus_fault"]
            faults.append(
                {
                    "trace": str(path),
                    "line": line_number,
                    "kind": fault.get("kind", "unknown"),
                    "address": int(fault.get("address", 0)),
                    "size": int(fault.get("size", 0)),
                    "status": fault.get("status", record.get("status_text", "")),
                    "region": fault.get("region", ""),
                    "symbol": fault.get("symbol", ""),
                }
            )
    return faults


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
                    "first": {
                        "trace": fault["trace"],
                        "line": fault["line"],
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

    rows = []
    for item in groups.values():
        rows.append(
            {
                "kind": item["kind"],
                "address": item["address"],
                "address_hex": item["address_hex"],
                "size": item["size"],
                "count": item["count"],
                "trace_count": len(item["traces"]),
                "region": (
                    item["regions"].most_common(1)[0][0]
                    if item["regions"]
                    else ""
                ),
                "symbol": (
                    item["symbols"].most_common(1)[0][0]
                    if item["symbols"]
                    else ""
                ),
                "statuses": dict(item["statuses"]),
                "first": item["first"],
                "traces": sorted(item["traces"]),
            }
        )
    rows.sort(key=lambda row: (-row["trace_count"], -row["count"], row["address"]))
    return rows


def render_markdown(rows) -> str:
    lines = [
        "# STF recovery fault summary",
        "",
        "| Traces | Hits | Kind | Address | Size | Region | Symbol | First trace |",
        "| ---: | ---: | --- | --- | ---: | --- | --- | --- |",
    ]
    for row in rows:
        first = f"{row['first']['trace']}:{row['first']['line']}"
        lines.append(
            f"| {row['trace_count']} | {row['count']} | {row['kind']} | "
            f"{row['address_hex']} | {row['size']} | {row['region'] or '-'} | "
            f"{row['symbol'] or '-'} | {first} |"
        )
    if not rows:
        lines.append("| - | - | - | - | - | - | - | no faults found |")
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
            f"region={row['region'] or '-'} symbol={row['symbol'] or '-'}"
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
