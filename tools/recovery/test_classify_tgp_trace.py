#!/usr/bin/env python3

from __future__ import annotations

import json
import tempfile
from pathlib import Path

from classify_tgp_trace import classify


def main() -> int:
    with tempfile.TemporaryDirectory() as temp_dir:
        trace = Path(temp_dir) / "trace.jsonl"
        trace.write_text(
            "\n".join(
                [
                    json.dumps(
                        {
                            "type": "memory",
                            "kind": "write",
                            "address": 0x00884000,
                            "bytes": "1a1a000d",
                        }
                    ),
                    json.dumps(
                        {
                            "type": "memory",
                            "kind": "write",
                            "address": 0x00884000,
                            "bytes": "0000803f",
                        }
                    ),
                ]
            )
            + "\n",
            encoding="utf-8",
        )

        report = classify(
            trace,
            0x00884000,
            0x00800000,
            0x00804000,
            set(),
        )

    if report["fifo_writes"] != 2:
        return 1
    if report["known_commands"] != [
        {"command": "scalar_sqrt", "count": 1}
    ]:
        return 2
    if report["fifo_sample"][0] != "0x0D001A1A":
        return 3
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
