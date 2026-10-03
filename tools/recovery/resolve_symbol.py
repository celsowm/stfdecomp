#!/usr/bin/env python3
"""
Normalize symbol listings produced by nm960/GNU nm/linker map files.

Examples:

    nm960 -n temp/rom_code1.out > build/rom_code1.nm
    python tools/recovery/resolve_symbol.py build/rom_code1.nm camera_init

    python tools/recovery/resolve_symbol.py build/rom_code1.nm --json build/rom_code1.symbols.json
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


NM_RE = re.compile(
    r"^\s*(?:0x)?([0-9A-Fa-f]{1,8})\s+[A-Za-z?]\s+([^\s]+)\s*$"
)
MAP_ASSIGN_RE = re.compile(
    r"^\s*([^\s=]+)\s*=\s*(0x[0-9A-Fa-f]+|[0-9A-Fa-f]{4,8})\s*;?\s*$"
)
MAP_ADDR_NAME_RE = re.compile(
    r"^\s*(0x[0-9A-Fa-f]+|[0-9A-Fa-f]{4,8})\s+([^\s]+)\s*$"
)


def parse_address(text: str) -> int:
    text = text.strip()
    if text.lower().startswith("0x"):
        return int(text, 16)
    return int(text, 16)


def parse_symbols(path: Path) -> dict[str, int]:
    result: dict[str, int] = {}

    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = NM_RE.match(line)
        if match:
            address, name = match.groups()
            result[name] = int(address, 16)
            continue

        match = MAP_ASSIGN_RE.match(line)
        if match:
            name, address = match.groups()
            result[name] = parse_address(address)
            continue

        match = MAP_ADDR_NAME_RE.match(line)
        if match:
            address, name = match.groups()
            result[name] = parse_address(address)

    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("symbols", type=Path)
    parser.add_argument("name", nargs="?")
    parser.add_argument("--json", dest="json_output", type=Path)
    parser.add_argument("--prefix", help="list symbols starting with PREFIX")
    args = parser.parse_args()

    symbols = parse_symbols(args.symbols)
    if not symbols:
        raise SystemExit(f"no symbols recognized in {args.symbols}")

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            name: f"0x{address:08X}"
            for name, address in sorted(symbols.items(), key=lambda item: (item[1], item[0]))
        }
        args.json_output.write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")

    if args.name:
        if args.name not in symbols:
            raise SystemExit(f"symbol not found: {args.name}")
        print(f"0x{symbols[args.name]:08X}")
        return 0

    if args.prefix is not None:
        matches = [
            (name, address)
            for name, address in symbols.items()
            if name.startswith(args.prefix)
        ]
        for name, address in sorted(matches, key=lambda item: (item[1], item[0])):
            print(f"0x{address:08X} {name}")
        return 0

    if not args.json_output:
        for name, address in sorted(symbols.items(), key=lambda item: (item[1], item[0])):
            print(f"0x{address:08X} {name}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
