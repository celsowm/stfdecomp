#!/usr/bin/env python3
"""
Compare two recovery state snapshots deterministically.

Snapshots are JSON objects. Nested dictionaries/lists are flattened to stable
paths, integer differences are shown in hexadecimal, and include/exclude regex
filters allow a recovery milestone to compare only state that is part of its
current contract.
"""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


def flatten(value, prefix=""):
    result = {}
    if isinstance(value, dict):
        for key in sorted(value):
            child = f"{prefix}.{key}" if prefix else str(key)
            result.update(flatten(value[key], child))
    elif isinstance(value, list):
        for index, item in enumerate(value):
            child = f"{prefix}[{index}]"
            result.update(flatten(item, child))
    else:
        result[prefix] = value
    return result


def display(value):
    if isinstance(value, int) and not isinstance(value, bool):
        return f"0x{value:X} ({value})"
    return repr(value)


def compile_patterns(values):
    return [re.compile(value) for value in values or []]


def accepted(path, includes, excludes):
    if includes and not any(pattern.search(path) for pattern in includes):
        return False
    if any(pattern.search(path) for pattern in excludes):
        return False
    return True


def compare(reference, recovered, includes=None, excludes=None):
    left = flatten(reference)
    right = flatten(recovered)
    include_patterns = compile_patterns(includes)
    exclude_patterns = compile_patterns(excludes)
    differences = []

    for path in sorted(set(left) | set(right)):
        if not accepted(path, include_patterns, exclude_patterns):
            continue
        left_present = path in left
        right_present = path in right
        if not left_present or not right_present or left[path] != right[path]:
            differences.append(
                {
                    "path": path,
                    "reference_present": left_present,
                    "recovered_present": right_present,
                    "reference": left.get(path),
                    "recovered": right.get(path),
                }
            )
    return differences


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("recovered", type=Path)
    parser.add_argument("--include", action="append", default=[])
    parser.add_argument("--exclude", action="append", default=[])
    parser.add_argument("--json", dest="json_output", type=Path)
    args = parser.parse_args()

    reference = json.loads(args.reference.read_text(encoding="utf-8"))
    recovered = json.loads(args.recovered.read_text(encoding="utf-8"))
    differences = compare(reference, recovered, args.include, args.exclude)

    if differences:
        print(f"MISMATCH: {len(differences)} differing state paths")
        for item in differences[:200]:
            print(
                f"{item['path']}: reference={display(item['reference'])} "
                f"recovered={display(item['recovered'])}"
            )
        if len(differences) > 200:
            print(f"... {len(differences) - 200} additional differences")
    else:
        print("MATCH")

    if args.json_output:
        args.json_output.parent.mkdir(parents=True, exist_ok=True)
        args.json_output.write_text(
            json.dumps({"differences": differences}, indent=2) + "\n",
            encoding="utf-8",
        )
    return 1 if differences else 0


if __name__ == "__main__":
    raise SystemExit(main())
