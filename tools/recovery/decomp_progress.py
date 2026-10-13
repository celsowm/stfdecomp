#!/usr/bin/env python3
"""Inventory STF program Assembly and explicitly evidenced partial C corridors.

A candidate entry label is NOT proof of a recovered/fully decoded function.
The result never claims a whole-game completion percentage.
"""
import argparse
import csv
import json
import re
from pathlib import Path

ASM_FILES = ("rom_code1.s", "rom_code2.s", "rom_code3.s", "rom_code4.s")
LABEL = re.compile(r"^\s*([A-Za-z_$][\w.$]*):{1,2}(?!:)")
CALL = re.compile(r"\b(?:call|bal)\s+([A-Za-z_$][\w.$]*)\b", re.I)
NUMERIC_FUNCTION = re.compile(r"^(?:sub|func)_([0-9a-f]{4,8})$", re.I)
LOCAL_OR_DATA = re.compile(
    r"^(?:loc|locret|off|byte|word|dword|qword|flt|dbl|asc|unk|"
    r"algn|jpt|stru|table|data|seg)_", re.I
)
NM_TEXT = re.compile(r"^\s*([0-9a-fA-F]+)\s+[TtWw]\s+([A-Za-z_$][\w.$]*)\s*$")
SPACE_SIZE = 0x100000


def scan_asm(path):
    labels, calls = {}, set()
    text_section = True
    line_count = 0
    with path.open(encoding="utf-8", errors="replace") as stream:
        for no, raw in enumerate(stream, 1):
            line_count = no
            code = raw.split("#", 1)[0].split("//", 1)[0].strip()
            if not code:
                continue
            if re.match(r"^\.(?:section\s+)?\.?data\b", code):
                text_section = False
            elif re.match(r"^\.(?:section\s+)?\.?text\b", code):
                text_section = True
            match = LABEL.match(code)
            if match:
                name = match.group(1)
                m = NUMERIC_FUNCTION.match(name)
                labels[name] = {
                    "symbol": name, "file": path.name, "line": no,
                    "section": "text" if text_section else "data",
                    "address": int(m.group(1), 16) if m else None,
                    "address_basis": "numeric_label" if m else None,
                }
            if text_section:
                calls.update(m.group(1) for m in CALL.finditer(code))
    return labels, calls, line_count


def union_size(ranges):
    total, end = 0, 0
    for lo, hi in sorted(ranges):
        if hi <= end:
            continue
        total += hi - max(lo, end)
        end = hi
    return total


def audit(root, evidence, nm=None):
    symbols, targets, stats = {}, set(), {}
    for filename in ASM_FILES:
        p = root / "src/asm" / filename
        labels, calls, lines = scan_asm(p)
        symbols.update(labels)
        targets.update(calls)
        stats[filename] = {"lines": lines, "labels": len(labels)}

    addresses = {}
    if nm is not None:
        for line in nm.read_text(encoding="utf-8").splitlines():
            match = NM_TEXT.match(line)
            if match:
                addresses[match.group(2)] = int(match.group(1), 16)
    for name, item in symbols.items():
        if name in addresses:
            item["address"] = addresses[name]
            item["address_basis"] = "nm_text"
        is_function = (
            item["section"] == "text"
            and not LOCAL_OR_DATA.match(name)
            and (name in targets or bool(NUMERIC_FUNCTION.match(name)))
        )
        item["candidate"] = is_function
        item["reason"] = (
            "direct_call" if name in targets else
            "numeric_function_label" if NUMERIC_FUNCTION.match(name) else None
        )
        item["mapped_evidence_ids"] = []
        item["status"] = "unmapped"

    raw = json.loads(evidence.read_text(encoding="utf-8"))
    if raw.get("schema_version") != 1 or not isinstance(raw.get("corridors"), list):
        raise ValueError("expected schema_version=1 and corridors[]")
    corridors, seen = [], set()
    for record in raw["corridors"]:
        key = record["id"]
        if key in seen:
            raise ValueError(f"duplicate evidence id: {key}")
        seen.add(key)
        lo, hi = int(record["start"], 0), int(record["end"], 0)
        if not 0 <= lo < hi <= SPACE_SIZE:
            raise ValueError(f"invalid original ROM bounds for {key}")
        if record["status"] != "partial":
            raise ValueError(f"only documented partial corridors are credited: {key}")
        if not record.get("c_sources") or not record.get("reference"):
            raise ValueError(f"missing provenance for {key}")
        for path in [*record["c_sources"], record["reference"]]:
            if not (root / path).is_file():
                raise ValueError(f"missing evidence file {path}")
        mapped = set()
        for name in record.get("owner_symbols", []):
            if name in symbols and symbols[name]["candidate"]:
                mapped.add(name)
        for name, item in symbols.items():
            if item["candidate"] and item["address"] == lo:
                mapped.add(name)
        for name in mapped:
            symbols[name]["mapped_evidence_ids"].append(key)
            symbols[name]["status"] = "partial_C"
        corridors.append({
            "id": key, "start": lo, "end": hi, "bytes": hi - lo,
            "status": "partial", "c_sources": record["c_sources"],
            "reference": record["reference"], "owner_symbols": record.get("owner_symbols", []),
            "matched_asm_candidates": sorted(mapped),
        })

    functions = sorted(
        (item for item in symbols.values() if item["candidate"]),
        key=lambda item: (
            item["address"] is None,
            item["address"] if item["address"] is not None else 0,
            item["symbol"],
        ),
    )
    matched = sum(bool(item["mapped_evidence_ids"]) for item in functions)
    known_bytes = union_size([(x["start"], x["end"]) for x in corridors])
    return {
        "schema_version": 1,
        "meaning": (
            "Candidates are label/call-based and not an exhaustive function list. "
            "Signed ROM ranges are evidence of original instruction bytes, NOT "
            "equivalence with recovered C. Original Makefile still builds Assembly."
        ),
        "counts": {
            "source_asm_lines": sum(x["lines"] for x in stats.values()),
            "defined_asm_labels": len(symbols),
            "direct_call_targets": len(targets),
            "candidate_functions": len(functions),
            "candidate_functions_with_partial_C": matched,
            "candidate_functions_unmapped": len(functions) - matched,
            "byte_exact_complete_functions": 0,
            "documented_partial_corridors": len(corridors),
            "unique_corridor_rom_bytes": known_bytes,
            "nominal_rom_address_bytes": SPACE_SIZE,
        },
        "percentages": {
            "candidate_function_mapping": (
                round(100 * matched / len(functions), 4) if functions else None
            ),
            "documented_rom_address_fraction": round(100 * known_bytes / SPACE_SIZE, 4),
            "actual_full_game_decomp": None,
        },
        "asm_stats": stats,
        "nm_supplied": nm is not None,
        "corridors": corridors,
        "function_candidates": functions,
        "direct_calls_without_defined_symbol": sorted(targets - symbols.keys()),
    }


def to_md(result):
    c = result["counts"]
    p = result["percentages"]
    pc = p["candidate_function_mapping"]
    lines = [
        "# Sonic the Fighters: decompilation audit", "",
        "**No defensible complete-game percentage exists yet.** The candidate",
        "coverage below measures documented C links only, NOT working gameplay",
        "or C replacing original Assembly in the rebuilt ROM.", "",
        "| Metric | Measured |", "| --- | ---: |",
        f"| Assembly source lines | {c['source_asm_lines']:,} |",
        f"| Defined ASM labels | {c['defined_asm_labels']:,} |",
        f"| Named function candidates (direct calls/sub_ labels) | {c['candidate_functions']:,} |",
        f"| Candidate functions with evidenced partial C | {c['candidate_functions_with_partial_C']:,} |",
        f"| Partial-C mapping / identified candidates | {pc:.4f}% |" if pc is not None else "| Candidate mapping | N/A |",
        f"| Complete original functions independently byte-matched in rebuilt C ROM | {c['byte_exact_complete_functions']:,} |",
        f"| Curated partial ROM corridors | {c['documented_partial_corridors']:,} |",
        f"| Unique documented corridor bytes | {c['unique_corridor_rom_bytes']:,} / {SPACE_SIZE:,} |",
        f"| Documented corridor fraction of 1 MiB ROM address space | {p['documented_rom_address_fraction']:.4f}% |",
        "| Actual full decomp | **Not measurable** |", "",
        "## Documented partial C corridors", "",
        "| Name | Start | End (exclusive) | Bytes | Mapped function candidates |",
        "| --- | ---: | ---: | ---: | --- |",
    ]
    for item in result["corridors"]:
        names = ", ".join(item["matched_asm_candidates"]) or "not resolved"
        lines.append(
            f"| {item['id']} | 0x{item['start']:06X} | 0x{item['end']:06X} | "
            f"{item['bytes']} | {names} |"
        )
    lines += [
        "", "## Measurement limitations", "",
        "- Named function candidates exclude indirect calls, unnamed routines and",
        "  unresolved data-driven task entrypoints; some direct branches may be",
        "  mislabeled. This is NOT a complete original function count.",
        "- Numeric sub_ labels give candidate addresses; symbol addresses from",
        "  a linked executable can be added with --nm.",
        "- ROM hashes validate which original bytes a corridor references,",
        "  not whether its C runtime matches all branches, timing or state.",
        "- Byte totals union overlapping ranges. Nominal 1 MiB includes data",
        "  and is NOT the number of executable-code bytes.",
        "- Complete decompilation requires a comprehensive function inventory,",
        "  original-vs-C differential tests, and a C-linked ROM rebuild.",
        "",
        "See decomp_function_inventory.csv and decomp_progress.json in the artifact.",
        "",
    ]
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--nm", type=Path, help="Optional linked ROM nm output")
    parser.add_argument("--out-dir", type=Path, default=Path("build/decomp-progress"))
    args = parser.parse_args()
    root = args.root.resolve()
    manifest = args.manifest or root / "tools/recovery/decomp_evidence.json"
    result = audit(root, manifest, args.nm)
    args.out_dir.mkdir(parents=True, exist_ok=True)
    (args.out_dir / "decomp_progress.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )
    (args.out_dir / "DECOMP_PROGRESS.md").write_text(to_md(result), encoding="utf-8")
    with (args.out_dir / "decomp_function_inventory.csv").open(
        "w", newline="", encoding="utf-8"
    ) as out:
        fields = ["symbol", "file", "line", "section", "address",
                  "address_basis", "reason", "mapped_evidence_ids", "status"]
        writer = csv.DictWriter(out, fieldnames=fields)
        writer.writeheader()
        for entry in result["function_candidates"]:
            row = {name: entry.get(name) for name in fields}
            row["address"] = (
                "0x%08X" % entry["address"] if entry["address"] is not None else ""
            )
            row["mapped_evidence_ids"] = ";".join(entry["mapped_evidence_ids"])
            writer.writerow(row)
    print(to_md(result))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
