#!/usr/bin/env python3
"""
Extract STF task descriptors from the assembly task table.

The table records task-local allocation size, init entry and per-frame entry.
This tool reports those fields without guessing the runtime base/register that
points at each task instance.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


DESCRIPTOR_RE = re.compile(
    r"(?m)"
    r"^\s*\.long\s+(0x[0-9A-Fa-f]+|[0-9]+)(?:\s*#.*)?\n"
    r"^\s*\.long\s+([A-Za-z_.$][A-Za-z0-9_.$]*)(?:\s*#.*)?\n"
    r"^\s*\.long\s+([A-Za-z_.$][A-Za-z0-9_.$]*)(?:\s*#.*)?\n"
    r"^\s*\.long\s+([^\n#]+)(?:\s*#.*)?\n"
    r"^([A-Za-z_.$][A-Za-z0-9_.$]*):\s*\.asciz\s+\"([^\"]+)\""
)


def parse_int(text: str) -> int:
    return int(text, 0)


def normalize_name(text: str) -> str:
    return text.strip().rstrip("\x00").strip()


def extract(path: Path):
    source = path.read_text(encoding="utf-8", errors="replace")
    records = []

    for match in DESCRIPTOR_RE.finditer(source):
        size_text, init_symbol, update_symbol, aux_text, label, display = match.groups()
        task_name = normalize_name(display)
        if not task_name.startswith("fa_"):
            continue

        line = source.count("\n", 0, match.start()) + 1
        records.append(
            {
                "task": task_name,
                "label": label,
                "workspace_size": parse_int(size_text),
                "workspace_size_hex": f"0x{parse_int(size_text):X}",
                "init": init_symbol,
                "update": update_symbol,
                "aux": aux_text.strip(),
                "source": str(path),
                "line": line,
            }
        )

    records.sort(key=lambda item: (item["line"], item["task"]))
    return records


def render_markdown(records) -> str:
    lines = [
        "# Sonic the Fighters task descriptors",
        "",
        "Extracted mechanically from the assembly task table. Workspace size is",
        "reported as data, not interpreted as stack size or object-layout proof.",
        "",
        "| Task | Workspace | Init | Update | Source line |",
        "| --- | ---: | --- | --- | ---: |",
    ]
    for item in records:
        lines.append(
            f"| `{item['task']}` | `{item['workspace_size_hex']}` | "
            f"`{item['init']}` | `{item['update']}` | {item['line']} |"
        )
    if not records:
        lines.append("| - | - | - | - | - |")
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "assembly",
        nargs="?",
        type=Path,
        default=Path("src/asm/rom_code2.s"),
    )
    parser.add_argument("--json", dest="json_output", type=Path)
    parser.add_argument("--markdown", dest="markdown_output", type=Path)
    parser.add_argument("--task", help="show one task descriptor")
    args = parser.parse_args()

    records = extract(args.assembly)

    if args.task:
        records = [item for item in records if item["task"] == args.task]
        if not records:
            raise SystemExit(f"task not found: {args.task}")

    for item in records:
        print(
            f"{item['task']:<18} workspace={item['workspace_size_hex']:<8} "
            f"init={item['init']:<24} update={item['update']}"
        )

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(
            json.dumps({"tasks": records}, indent=2) + "\n",
            encoding="utf-8",
        )

    if args.markdown_output:
        args.markdown_output.parent.mkdir(parents=True, exist_ok=True)
        args.markdown_output.write_text(render_markdown(records), encoding="utf-8")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
