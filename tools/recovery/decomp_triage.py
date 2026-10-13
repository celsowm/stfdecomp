#!/usr/bin/env python3
"""Prioritize STF disassembly work without crediting hypothetical mappings.

Consumes decomp_progress.json from decomp_progress.py and inspects original
ASM for direct calls, data-pointer references and nearest known candidate
addresses.  All candidate C filename matches are explicitly *unverified*.
"""
from __future__ import annotations

import argparse
import csv
import json
import re
from collections import Counter
from pathlib import Path

ASM_FILES = ("rom_code1.s", "rom_code2.s", "rom_code3.s", "rom_code4.s")
CALL = re.compile(r"\b(?:call|bal)\s+([A-Za-z_$][\w.$]*)\b", re.IGNORECASE)
DATA_REF = re.compile(r"^\s*\.(?:long|word|short|int|4byte|8byte|quad)\s+(.+)$", re.I)
SYMBOL = re.compile(r"(?<![\w.$])([A-Za-z_$][\w.$]*)(?![\w.$])")
ROM_LIMIT = 0x100000

AREA_PREFIXES = (
    ("collision", ("coli", "collision", "no_coli", "crush", "damage")),
    ("fighting", ("attack", "hit", "kabe", "ring", "skill")),
    ("animation", ("motion", "kamae", "osage", "skeleton", "mot_")),
    ("graphics", ("draw", "disp", "polygon", "poly", "set_obj", "texture")),
    ("camera", ("cam", "camera", "focus")),
    ("audio", ("sound", "audio", "sd_")),
    ("gameflow", ("game", "round", "menu", "mode", "select")),
    ("input", ("input", "key_", "pad", "joy")),
)


def area(symbol: str) -> str:
    lower = symbol.lower()
    for group, stems in AREA_PREFIXES:
        if lower.startswith(stems):
            return group
    return "other"


def references(root: Path, symbols: set[str]) -> tuple[Counter, Counter, dict]:
    """Count callsites and assembler-defined pointer words separately."""
    direct, pointers = Counter(), Counter()
    examples = {}
    for asm in ASM_FILES:
        path = root / "src/asm" / asm
        with path.open(encoding="utf-8", errors="replace") as stream:
            for no, raw in enumerate(stream, 1):
                line = raw.split("#", 1)[0].split("//", 1)[0]
                for m in CALL.finditer(line):
                    if m.group(1) in symbols:
                        direct[m.group(1)] += 1
                m = DATA_REF.match(line)
                if m:
                    for token in SYMBOL.findall(m.group(1)):
                        if token in symbols:
                            pointers[token] += 1
                            examples.setdefault(token, [asm, no])
    return direct, pointers, examples


def build_triage(root: Path, progress: dict) -> dict:
    candidates = progress["function_candidates"]
    all_symbols = set()
    # The full defined label table was not serialized by the baseline.
    # Pointer analysis only *proposes* new entrypoints, and so scans every
    # ASM label independently, not only confirmed function candidates.
    label = re.compile(r"^\s*([A-Za-z_$][\w.$]*):{1,2}(?!:)")
    for fn in ASM_FILES:
        with (root / "src/asm" / fn).open(encoding="utf-8", errors="replace") as inp:
            for raw in inp:
                m = label.match(raw)
                if m:
                    all_symbols.add(m.group(1))
    direct, pointer, examples = references(root, all_symbols)
    cfiles = sorted((root / "src/recovered").glob("*.c"))
    stems = {p.stem.lower(): "src/recovered/" + p.name for p in cfiles}
    candidate_by_name = {r["symbol"] for r in candidates}
    entries = []
    known_address = sorted({
        r["address"] for r in candidates
        if isinstance(r["address"], int) and 0 <= r["address"] < ROM_LIMIT
    })
    from bisect import bisect_right

    for record in candidates:
        name = record["symbol"]
        addr = record["address"]
        next_addr = None
        if isinstance(addr, int) and 0 <= addr < ROM_LIMIT:
            idx = bisect_right(known_address, addr)
            if idx < len(known_address):
                next_addr = known_address[idx]
        hints = []
        if name.lower() in stems:
            hints.append(stems[name.lower()])
        # Prefix match can help review but is not enough to credit recovery.
        hints += [
            path for stem, path in stems.items()
            if stem.startswith(name.lower() + "_") and path not in hints
        ]
        is_mapped = bool(record["mapped_evidence_ids"])
        calls = direct[name]
        ptrs = pointer[name]
        entries.append({
            "symbol": name,
            "subsystem": area(name),
            "source_asm": record["file"],
            "source_line": record["line"],
            "start_address": addr,
            "address_origin": record["address_basis"],
            "next_known_candidate_start": next_addr,
            "direct_callsites": calls,
            "pointer_data_references": ptrs,
            "mapped_partial_C": is_mapped,
            "evidence_ids": record["mapped_evidence_ids"],
            "c_filename_hints_unverified": hints,
            "priority_score": 5 * calls + 2 * ptrs,
        })
    entries.sort(key=lambda r: (
        r["mapped_partial_C"],
        -r["priority_score"],
        -r["direct_callsites"],
        r["symbol"],
    ))

    indirect = []
    for symbol, count in pointer.items():
        if symbol in candidate_by_name:
            continue
        if count < 1:
            continue
        if symbol.lower().startswith((
            "loc_", "off_", "byte_", "word_", "dword_", "flt_", "dbl_",
            "asc_", "unk_", "algn_", "jpt_", "stru_", "data_",
        )):
            continue
        origin, line = examples[symbol]
        indirect.append({
            "symbol": symbol, "data_pointer_references": count,
            "asm_file": origin, "example_line": line,
            "classification": "pointer_target_requires_code_verification",
        })
    indirect.sort(key=lambda r: (-r["data_pointer_references"], r["symbol"]))
    summaries = {}
    for group in sorted({item["subsystem"] for item in entries}):
        subset = [item for item in entries if item["subsystem"] == group]
        summaries[group] = {
            "candidates": len(subset),
            "mapped_partial_C": sum(x["mapped_partial_C"] for x in subset),
            "direct_callsites": sum(x["direct_callsites"] for x in subset),
            "pointer_data_references": sum(x["pointer_data_references"] for x in subset),
        }
    return {
        "schema_version": 1,
        "warning": (
            "Priority score is a callsite heuristic, not percentage of game "
            "recovered, function duration or evidence of C correctness. "
            "Next known candidate start is NOT a verified function end. "
            "Pointer targets and filename similarities are unverified hypotheses."
        ),
        "stats": {
            "c_source_files": len(cfiles),
            "prioritized_candidates": len(entries),
            "candidates_with_inferred_next_start": sum(
                x["next_known_candidate_start"] is not None for x in entries
            ),
            "unverified_pointer_target_names": len(indirect),
            "unmapped_candidates_with_unverified_C_filename_hints": sum(
                not x["mapped_partial_C"] and bool(x["c_filename_hints_unverified"])
                for x in entries
            ),
        },
        "subsystems": summaries,
        "priority_queue": entries,
        "pointer_entry_hypotheses": indirect,
    }


def to_md(result: dict, top: int = 25) -> str:
    stats = result["stats"]
    output = [
        "# STF decompilation next-target queue", "",
        "> **Not a decompilation percentage.** Priority ranks come from direct",
        "> Assembly callsites and data-pointer references. C filename hints,",
        "> next candidate addresses and pointer entries are only hypotheses.",
        "",
        "## Triage inventory", "",
        "| Metric | Count |", "| --- | ---: |",
        f"| C source files in src/recovered | {stats['c_source_files']} |",
        f"| Original function candidates ranked | {stats['prioritized_candidates']} |",
        f"| Candidates with a following known start (NOT function length) | {stats['candidates_with_inferred_next_start']} |",
        f"| Unverified ASM pointer target names outside candidate inventory | {stats['unverified_pointer_target_names']} |",
        f"| Unmapped candidates with unverified filename hints | {stats['unmapped_candidates_with_unverified_C_filename_hints']} |",
        "", "## Review priorities (unmapped candidates)", "",
        "| Rank | ASM symbol | Area | Direct calls | Pointer refs | C filename hint? |",
        "| ---: | --- | --- | ---: | ---: | --- |",
    ]
    top_items = [x for x in result["priority_queue"] if not x["mapped_partial_C"]][:top]
    for i, item in enumerate(top_items, 1):
        output.append(
            f"| {i} | {item['symbol']} | {item['subsystem']} | "
            f"{item['direct_callsites']} | {item['pointer_data_references']} | "
            f"{', '.join(item['c_filename_hints_unverified']) or '—'} |"
        )
    output += [
        "", "## Areas", "",
        "| Area | Candidates | Partial-C links | Direct calls |",
        "| --- | ---: | ---: | ---: |",
    ]
    for group, values in result["subsystems"].items():
        output.append(
            f"| {group} | {values['candidates']} | "
            f"{values['mapped_partial_C']} | {values['direct_callsites']} |"
        )
    output += [
        "", "## Manual verification workflow", "",
        "1. Inspect a top-ranked Assembly symbol; validate whether the entry",
        "   is code and find its actual entry address and control-flow bounds.",
        "2. Verify pointer references using task tables and the decoded ROM",
        "   before adding any indirect-call target to the function inventory.",
        "3. Identify the corresponding recovered C code by semantics, not",
        "   similarity of file name, and add a bounded ROM evidence record.",
        "4. Validate CPU state, memory writes and TGP command traces before",
        "   upgrading any recovery status. Byte-matched C rebuild requires",
        "   independently reproduced full original instruction bytes.",
        "",
        "Detailed queues: decomp_priorities.csv and",
        "decomp_pointer_hypotheses.csv, plus machine-readable decomp_triage.json.",
        "",
    ]
    return "\n".join(output)


def main() -> None:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    cli.add_argument("--out-dir", type=Path, default=Path("build/decomp-progress"))
    args = cli.parse_args()
    root, out = args.root.resolve(), args.out_dir
    progress = json.loads((out / "decomp_progress.json").read_text(encoding="utf-8"))
    result = build_triage(root, progress)
    out.mkdir(parents=True, exist_ok=True)
    (out / "decomp_triage.json").write_text(
        json.dumps(result, indent=2) + "\n", encoding="utf-8"
    )
    (out / "DECOMP_TRIAGE.md").write_text(to_md(result), encoding="utf-8")
    for filename, rows, fields in (
        ("decomp_priorities.csv", result["priority_queue"], [
            "symbol", "subsystem", "source_asm", "source_line",
            "start_address", "address_origin", "next_known_candidate_start",
            "direct_callsites", "pointer_data_references", "mapped_partial_C",
            "evidence_ids", "c_filename_hints_unverified", "priority_score",
        ]),
        ("decomp_pointer_hypotheses.csv", result["pointer_entry_hypotheses"], [
            "symbol", "data_pointer_references", "asm_file",
            "example_line", "classification",
        ]),
    ):
        with (out / filename).open("w", newline="", encoding="utf-8") as fp:
            writer = csv.DictWriter(fp, fieldnames=fields)
            writer.writeheader()
            for item in rows:
                row = dict(item)
                for name in ("start_address", "next_known_candidate_start"):
                    if name in row and row[name] is not None:
                        row[name] = f"0x{row[name]:08X}"
                for name in ("evidence_ids", "c_filename_hints_unverified"):
                    if name in row:
                        row[name] = ";".join(row[name])
                writer.writerow(row)
    print(to_md(result))


if __name__ == "__main__":
    main()
