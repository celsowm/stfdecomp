#!/usr/bin/env python3
"""Compare ordered Model 2 TGP command/reply JSONL captures bit-for-bit.

Each nonblank line is a JSON object with opcode, arguments (1 or 2 raw
uint32 words) and reply (one raw IEEE-754 uint32). Example:
{"opcode":"0x12002424","arguments":["0x00000000","0x40000000"],"reply":"0x00000000"}

These are *observed* runtime transactions, not raw ROM instruction bytes.
No FIFO location, command ordering or TGP response is inferred from a ROM.
Exit codes: 0=identical; 1=divergence; 2=invalid trace/input.
"""

from __future__ import annotations

import argparse
import json
import math
import struct
from pathlib import Path


def uint32(raw: object) -> int:
    if isinstance(raw, bool):
        raise ValueError("boolean is not a uint32")
    if isinstance(raw, int):
        value = raw
    elif isinstance(raw, str):
        value = int(raw.strip(), 0)
    else:
        raise ValueError(f"uint32 must be integer or string, got {type(raw).__name__}")
    if not 0 <= value <= 0xFFFFFFFF:
        raise ValueError(f"uint32 outside range: {value}")
    return value


def f32(bits: int) -> float:
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def ordered_float(bits: int) -> int:
    return (~bits & 0xFFFFFFFF) if (bits & 0x80000000) else (bits | 0x80000000)


def load(path: Path) -> list[dict]:
    entries: list[dict] = []
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip():
            continue
        try:
            obj = json.loads(line)
            if not isinstance(obj, dict):
                raise ValueError("expected JSON object")
            args = obj["arguments"]
            if not isinstance(args, list) or len(args) not in (1, 2):
                raise ValueError("arguments must contain one or two words")
            if "argument_count" in obj and uint32(obj["argument_count"]) != len(args):
                raise ValueError("argument_count does not match arguments")
            entry = {
                "opcode": uint32(obj["opcode"]),
                "arguments": [uint32(value) for value in args],
                "reply": uint32(obj["reply"]),
            }
            if not math.isfinite(f32(entry["reply"])):
                raise ValueError("nonfinite reply is not a successful TGP transaction")
        except (ValueError, TypeError, KeyError, json.JSONDecodeError) as exc:
            raise ValueError(f"{path}:{number}: {exc}") from exc
        entries.append(entry)
    if not entries:
        raise ValueError(f"{path}: empty command transcript")
    return entries


def compare(reference: list[dict], candidate: list[dict]) -> dict:
    for index, (expected, actual) in enumerate(zip(reference, candidate)):
        for key in ("opcode", "arguments", "reply"):
            if expected[key] != actual[key]:
                report = {
                    "identical": False,
                    "index": index,
                    "kind": "reply" if key == "reply" else "input",
                    "field": key,
                    "reference": expected,
                    "candidate": actual,
                    "reference_count": len(reference),
                    "candidate_count": len(candidate),
                }
                if key == "reply":
                    a, b = expected[key], actual[key]
                    report["ulp_distance"] = abs(ordered_float(a) - ordered_float(b))
                    report["absolute_float_delta"] = abs(f32(a) - f32(b))
                return report
    if len(reference) != len(candidate):
        return {
            "identical": False,
            "index": min(len(reference), len(candidate)),
            "kind": "length",
            "reference_count": len(reference),
            "candidate_count": len(candidate),
        }
    return {"identical": True, "commands": len(reference)}


def load_vector(path: Path) -> dict[str, int]:
    """Read a post-CPU-multiplier X/Y/Z snapshot as raw finite binary32 words."""
    try:
        obj = json.loads(path.read_text(encoding="utf-8"))
        if not isinstance(obj, dict):
            raise ValueError("expected JSON object with x, y, z")
        vector = {axis: uint32(obj[axis]) for axis in ("x", "y", "z")}
        for axis, raw in vector.items():
            if not math.isfinite(f32(raw)):
                raise ValueError(f"{axis} is nonfinite")
    except (ValueError, TypeError, KeyError, json.JSONDecodeError, OSError) as exc:
        raise ValueError(f"{path}: invalid post-multiplier vector: {exc}") from exc
    return vector


def compare_vectors(reference: dict[str, int], candidate: dict[str, int]) -> dict:
    """Bit-exact final impact-vector comparison with axis and ULP diagnostics."""
    axes = ("x", "y", "z")
    mismatch_mask = 0
    ulps = {}
    for bit, axis in enumerate(axes):
        before, after = reference[axis], candidate[axis]
        if before != after:
            mismatch_mask |= 1 << bit
        ulps[axis] = abs(ordered_float(before) - ordered_float(after))
    return {
        "identical": mismatch_mask == 0,
        "mismatch_mask": mismatch_mask,
        "ulp_distance": ulps,
        "reference": {axis: f"0x{reference[axis]:08X}" for axis in axes},
        "candidate": {axis: f"0x{candidate[axis]:08X}" for axis in axes},
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("candidate", type=Path)
    parser.add_argument("--json", dest="json_path", type=Path)
    parser.add_argument("--reference-vector", type=Path,
                        help="JSON with captured post-multiplier x/y/z raw words")
    parser.add_argument("--candidate-vector", type=Path,
                        help="JSON with recovered post-multiplier x/y/z raw words")
    args = parser.parse_args(argv)

    if (args.reference_vector is None) != (args.candidate_vector is None):
        print("INVALID: --reference-vector and --candidate-vector must be used together")
        return 2

    try:
        report = compare(load(args.reference), load(args.candidate))
        if args.reference_vector is not None:
            vectors = compare_vectors(
                load_vector(args.reference_vector),
                load_vector(args.candidate_vector),
            )
            report["vector_comparison"] = vectors
            report["transcript_identical"] = report["identical"]
            report["identical"] = report["identical"] and vectors["identical"]
    except (OSError, ValueError) as exc:
        print(f"INVALID: {exc}")
        return 2

    if report.get("transcript_identical", report["identical"]):
        print(f"MATCH: {report['commands']} ordered command/reply tuples match bit-for-bit")
    else:
        print(f"DIFFERENCE at command {report['index']}: {report['kind']}")
        if report["kind"] != "length":
            print(f"  reference: {report['reference']}")
            print(f"  candidate: {report['candidate']}")
            if report["kind"] == "reply":
                print(f"  reply ULP distance: {report['ulp_distance']}")
                print(f"  absolute float delta: {report['absolute_float_delta']}")
        else:
            print(f"  reference count: {report['reference_count']}")
            print(f"  candidate count: {report['candidate_count']}")

    if "vector_comparison" in report:
        v = report["vector_comparison"]
        if v["identical"]:
            print("VECTORS MATCH: all post-multiplier X/Y/Z words bit-identical")
        else:
            print(f"VECTOR DIFFERENCE: axis mask 0x{v['mismatch_mask']:X}")
            print(f"  reference: {v['reference']}")
            print(f"  candidate: {v['candidate']}")
            print(f"  ULP distances: {v['ulp_distance']}")

    if args.json_path is not None:
        try:
            args.json_path.parent.mkdir(parents=True, exist_ok=True)
            args.json_path.write_text(
                json.dumps(report, indent=2) + "\n", encoding="utf-8"
            )
        except OSError as exc:
            print(f"INVALID: cannot write JSON report: {exc}")
            return 2
    return 0 if report["identical"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
