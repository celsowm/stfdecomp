#!/usr/bin/env python3
"""Regression for strict JSONL comparison; all replies are synthetic fixtures."""

from __future__ import annotations

import contextlib
import io
import json
import tempfile
from pathlib import Path

from compare_tgp_command_traces import compare, load, main, uint32


def write(path: Path, entries: list[dict]) -> None:
    path.write_text(
        "\n".join(json.dumps(entry) for entry in entries) + "\n",
        encoding="utf-8",
    )


def check() -> None:
    sample = [
        {"opcode": "0x0D001A1A", "arguments": ["0x40000000"], "reply": "0x40000000"},
        {"opcode": "0x12002424", "arguments": ["0x0", "0x40000000"], "reply": "0x3F800000"},
        {"opcode": "0x12802525", "arguments": ["0x0", "0x40000000"], "reply": "0x40000000"},
    ]
    with tempfile.TemporaryDirectory() as temp:
        folder = Path(temp)
        reference, candidate = folder / "ref.jsonl", folder / "candidate.jsonl"
        report = folder / "report.json"
        write(reference, sample)
        write(candidate, sample)
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate), "--json", str(report)]) == 0
        assert json.loads(report.read_text())["commands"] == 3
        assert compare(load(reference), load(candidate))["identical"]
        assert uint32("0xFFFFFFFF") == 0xFFFFFFFF

        altered = [dict(entry) for entry in sample]
        altered[1]["reply"] = "0x3F800001"
        write(candidate, altered)
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate), "--json", str(report)]) == 1
        diff = json.loads(report.read_text())
        assert diff["index"] == 1 and diff["kind"] == "reply"
        assert diff["ulp_distance"] == 1 and diff["absolute_float_delta"] > 0

        altered[1]["reply"] = sample[1]["reply"]
        altered[2]["arguments"] = ["0x1", "0x40000000"]
        write(candidate, altered)
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate)]) == 1
        assert compare(load(reference), load(candidate))["kind"] == "input"

        write(candidate, altered[:2])
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate)]) == 1
        assert compare(load(reference), load(candidate))["kind"] == "length"

        invalid = [dict(entry) for entry in sample]
        invalid[0]["reply"] = "0x7FC00000"
        write(candidate, invalid)
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate)]) == 2

        invalid[0]["reply"] = sample[0]["reply"]
        invalid[1]["arguments"] = ["0x0"]
        invalid[1]["argument_count"] = 2
        write(candidate, invalid)
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate)]) == 2

        candidate.write_text("not-json\n", encoding="utf-8")
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate)]) == 2

        candidate.write_text("", encoding="utf-8")
        with contextlib.redirect_stdout(io.StringIO()):
            assert main([str(reference), str(candidate)]) == 2


if __name__ == "__main__":
    check()
