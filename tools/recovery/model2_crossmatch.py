#!/usr/bin/env python3
"""
Cross-title Sega Model 2 recovery matcher.

Compares recovered/native C regions from vf2-decomp with STF i960 assembly
regions without assuming that function addresses or names survived between
titles. Linker-script symbols are normalized to absolute addresses before
scoring candidates.

No ROM data is read or emitted.
"""

from __future__ import annotations

import argparse
import collections
import dataclasses
import json
import math
import re
from pathlib import Path
from typing import Iterable

HEX_RE = re.compile(r"0x[0-9A-Fa-f]{4,8}")
IDENT_RE = re.compile(r"\b[A-Za-z_.$][A-Za-z0-9_.$]*\b")
LD_ASSIGN_RE = re.compile(
    r"^\s*([A-Za-z_.$][A-Za-z0-9_.$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;",
    re.MULTILINE,
)
GLOBAL_RE = re.compile(r"^\s*\.global\s+([A-Za-z_.$][A-Za-z0-9_.$]*)\s*$", re.MULTILINE)
DEFINE_RE = re.compile(
    r"^\s*#\s*define\s+([A-Za-z_][A-Za-z0-9_]*)\s+"
    r"(?:UINT32_C\()?\s*(0x[0-9A-Fa-f]+)",
    re.MULTILINE,
)
ENUM_ASSIGN_RE = re.compile(
    r"\b([A-Za-z_][A-Za-z0-9_]*)\s*=\s*"
    r"(?:UINT32_C\()?\s*(0x[0-9A-Fa-f]+)"
)
ASM_LABEL_RE = re.compile(r"^([A-Za-z_.$][A-Za-z0-9_.$]*):\s*$", re.MULTILINE)
C_FUNCTION_RE = re.compile(
    r"(?m)^\s*(?:static\s+)?(?:inline\s+)?"
    r"(?:[A-Za-z_][A-Za-z0-9_]*\s+)+"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*\([^;{}]*\)\s*\{"
)

KNOWN_ANCHORS = {
    0x020E0004: "polygon/object table base",
    0x00501004: "geometry command/buffer state",
    0x00501008: "geometry maximum/buffer metric",
    0x00501018: "polygon/geometry limit gate",
    0x0050101C: "polygon/geometry accumulator gate",
    0x00501084: "horizontal focus distance",
    0x00501088: "vertical focus distance",
}


@dataclasses.dataclass(frozen=True)
class Region:
    title: str
    path: str
    name: str
    addresses: frozenset[int]

    @property
    def key(self) -> str:
        return f"{self.path}:{self.name}"


@dataclasses.dataclass(frozen=True)
class Match:
    vf2: Region
    stf: Region
    shared: tuple[int, ...]
    score: float


def is_model2_anchor(value: int) -> bool:
    return (
        0x00500000 <= value <= 0x005FFFFF
        or 0x00800000 <= value <= 0x00FFFFFF
        or 0x01000000 <= value <= 0x03FFFFFF
    )


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def parse_linker_symbols(root: Path) -> dict[str, int]:
    symbols: dict[str, int] = {}
    for path in root.rglob("*.ld"):
        for name, raw in LD_ASSIGN_RE.findall(read_text(path)):
            symbols[name] = int(raw, 16)
    return symbols


def parse_global_names(root: Path) -> set[str]:
    names: set[str] = set()
    include_root = root / "src" / "include"
    if not include_root.exists():
        return names
    for path in include_root.rglob("*"):
        if path.is_file() and path.suffix in {".s", ".S"}:
            names.update(GLOBAL_RE.findall(read_text(path)))
    return names


def parse_c_constants(text: str) -> dict[str, int]:
    constants: dict[str, int] = {}
    for pattern in (DEFINE_RE, ENUM_ASSIGN_RE):
        for name, raw in pattern.findall(text):
            constants[name] = int(raw, 16)
    return constants


def collect_c_constants(root: Path) -> dict[str, int]:
    constants: dict[str, int] = {}
    roots = [root / "include", root / "src" / "recovered", root / "src" / "hardware"]
    for base in roots:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.is_file() and path.suffix in {".h", ".c", ".inc"}:
                constants.update(parse_c_constants(read_text(path)))
    return constants


def addresses_in(text: str, symbols: dict[str, int]) -> frozenset[int]:
    addresses = {int(raw, 16) for raw in HEX_RE.findall(text)}
    for identifier in IDENT_RE.findall(text):
        value = symbols.get(identifier)
        if value is not None:
            addresses.add(value)
    return frozenset(value for value in addresses if is_model2_anchor(value))


def split_asm_regions(
    path: Path,
    root: Path,
    linker_symbols: dict[str, int],
    exported_names: set[str],
) -> list[Region]:
    text = read_text(path)
    labels = list(ASM_LABEL_RE.finditer(text))
    if not labels:
        return []

    selected = [
        label for label in labels
        if (
            label.group(1) in exported_names
            if exported_names
            else not label.group(1).startswith((".L", "$"))
        )
    ]
    if not selected:
        selected = [
            label for label in labels
            if not label.group(1).startswith((".L", "$"))
        ]

    regions: list[Region] = []
    for index, label in enumerate(selected):
        name = label.group(1)
        end = selected[index + 1].start() if index + 1 < len(selected) else len(text)
        body = text[label.start():end]
        addresses = addresses_in(body, linker_symbols)
        if addresses:
            regions.append(Region("stf", str(path.relative_to(root)), name, addresses))
    return regions


def matching_brace_end(text: str, brace_start: int) -> int:
    depth = 0
    in_string = False
    quote = ""
    escaped = False
    for index in range(brace_start, len(text)):
        char = text[index]
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                in_string = False
            continue
        if char in {'"', "'"}:
            in_string = True
            quote = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index + 1
    return len(text)


def split_c_regions(
    path: Path,
    root: Path,
    global_symbols: dict[str, int],
) -> list[Region]:
    text = read_text(path)
    symbols = dict(global_symbols)
    symbols.update(parse_c_constants(text))
    regions: list[Region] = []
    for match in C_FUNCTION_RE.finditer(text):
        brace = text.find("{", match.start())
        body = text[match.start():matching_brace_end(text, brace)]
        addresses = addresses_in(body, symbols)
        if addresses:
            regions.append(
                Region("vf2", str(path.relative_to(root)), match.group(1), addresses)
            )
    return regions


def collect_vf2_regions(root: Path) -> list[Region]:
    regions: list[Region] = []
    symbols = collect_c_constants(root)
    for base in (root / "src" / "recovered", root / "src" / "hardware"):
        if not base.exists():
            continue
        for path in base.rglob("*.c"):
            regions.extend(split_c_regions(path, root, symbols))
    return regions


def collect_stf_regions(root: Path) -> list[Region]:
    symbols = parse_linker_symbols(root)
    exported_names = parse_global_names(root)
    regions: list[Region] = []
    base = root / "src" / "asm"
    if not base.exists():
        return regions
    for path in sorted(base.rglob("*")):
        if path.is_file() and path.suffix in {".s", ".S"}:
            regions.extend(split_asm_regions(path, root, symbols, exported_names))
    return regions


def address_document_frequency(regions: Iterable[Region]) -> collections.Counter[int]:
    result: collections.Counter[int] = collections.Counter()
    for region in regions:
        result.update(region.addresses)
    return result


def score_matches(
    vf2_regions: list[Region],
    stf_regions: list[Region],
    min_shared: int,
    top: int,
) -> list[Match]:
    all_regions = vf2_regions + stf_regions
    frequency = address_document_frequency(all_regions)
    total = max(1, len(all_regions))

    def weight(address: int) -> float:
        rarity = 1.0 + math.log2((total + 1.0) / (frequency[address] + 1.0))
        if address in KNOWN_ANCHORS:
            rarity *= 1.35
        return rarity

    results: list[Match] = []
    for vf2 in vf2_regions:
        vf2_weight = sum(weight(a) for a in vf2.addresses)
        candidates: list[Match] = []
        for stf in stf_regions:
            shared = vf2.addresses & stf.addresses
            if len(shared) < min_shared:
                continue
            stf_weight = sum(weight(a) for a in stf.addresses)
            shared_weight = sum(weight(a) for a in shared)
            denominator = math.sqrt(max(vf2_weight, 1e-9) * max(stf_weight, 1e-9))
            score = min(100.0, 100.0 * shared_weight / denominator)
            candidates.append(Match(vf2, stf, tuple(sorted(shared)), score))
        candidates.sort(key=lambda item: (-item.score, -len(item.shared), item.stf.key))
        results.extend(candidates[:top])
    results.sort(key=lambda item: (-item.score, -len(item.shared), item.vf2.key, item.stf.key))
    return results


def reverse_linker_map(root: Path) -> dict[int, list[str]]:
    reverse: dict[int, list[str]] = collections.defaultdict(list)
    for name, value in parse_linker_symbols(root).items():
        reverse[value].append(name)
    for names in reverse.values():
        names.sort()
    return dict(reverse)


def anchor_report(stf_root: Path) -> list[dict[str, object]]:
    reverse = reverse_linker_map(stf_root)
    return [
        {
            "address": address,
            "hex": f"0x{address:08X}",
            "meaning": meaning,
            "stf_symbols": reverse.get(address, []),
        }
        for address, meaning in sorted(KNOWN_ANCHORS.items())
    ]


def render_markdown(matches: list[Match], anchors: list[dict[str, object]]) -> str:
    lines = [
        "# VF2 <-> Sonic the Fighters cross-title report",
        "",
        "Generated by tools/recovery/model2_crossmatch.py.",
        "",
        "Scores are heuristic evidence only. Candidates still require controlled",
        "tracing or differential validation before semantic renaming.",
        "",
        "## Shared runtime anchors",
        "",
        "| Address | STF symbol(s) | Evidence seed |",
        "| --- | --- | --- |",
    ]
    for row in anchors:
        names = ", ".join(row["stf_symbols"]) or "-"
        lines.append(f"| {row['hex']} | {names} | {row['meaning']} |")

    lines.extend(
        [
            "",
            "## Candidate regions",
            "",
            "| Score | Shared | VF2 recovered region | STF assembly candidate | Anchors |",
            "| ---: | ---: | --- | --- | --- |",
        ]
    )
    for match in matches:
        anchors_text = ", ".join(f"0x{value:08X}" for value in match.shared)
        lines.append(
            f"| {match.score:.1f} | {len(match.shared)} | "
            f"{match.vf2.key} | {match.stf.key} | {anchors_text} |"
        )
    if not matches:
        lines.append("| - | - | - | - | No candidates met the threshold |")
    lines.append("")
    return "\n".join(lines)


def render_json(matches: list[Match], anchors: list[dict[str, object]]) -> str:
    return json.dumps(
        {
            "anchors": anchors,
            "matches": [
                {
                    "score": round(match.score, 4),
                    "shared_count": len(match.shared),
                    "shared_addresses": [f"0x{value:08X}" for value in match.shared],
                    "vf2": {"path": match.vf2.path, "name": match.vf2.name},
                    "stf": {"path": match.stf.path, "name": match.stf.name},
                }
                for match in matches
            ],
        },
        indent=2,
    ) + "\n"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vf2-root", type=Path, default=Path("../vf2-decomp"))
    parser.add_argument("--stf-root", type=Path, default=Path("."))
    parser.add_argument("--min-shared", type=int, default=2)
    parser.add_argument("--top", type=int, default=5, help="candidates per VF2 region")
    parser.add_argument("--format", choices=("markdown", "json"), default="markdown")
    parser.add_argument("--output", type=Path)
    return parser


def main() -> int:
    args = build_parser().parse_args()
    vf2_root = args.vf2_root.resolve()
    stf_root = args.stf_root.resolve()

    if not (vf2_root / "README.md").exists():
        raise SystemExit(f"VF2 checkout not found: {vf2_root}")
    if not (stf_root / "src" / "asm").exists():
        raise SystemExit(f"STF checkout not found: {stf_root}")

    vf2_regions = collect_vf2_regions(vf2_root)
    stf_regions = collect_stf_regions(stf_root)
    matches = score_matches(vf2_regions, stf_regions, args.min_shared, args.top)
    anchors = anchor_report(stf_root)

    output = (
        render_markdown(matches, anchors)
        if args.format == "markdown"
        else render_json(matches, anchors)
    )
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output, encoding="utf-8")
    else:
        print(output, end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
