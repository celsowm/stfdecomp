#!/usr/bin/env python3

import json
import tempfile
import unittest
from pathlib import Path

import run_scenario


class ScenarioExpectationTests(unittest.TestCase):
    def test_state_and_work_ram_expectations(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            state = root / "state.json"
            work_ram = root / "workram.bin"

            state.write_text(
                json.dumps(
                    {
                        "cpu": {
                            "local_frame_depth": 0,
                            "procedure_calls": 1,
                            "procedure_returns": 1,
                        },
                        "probe": {
                            "accepted_device_accesses": 0,
                            "exploratory": False,
                        },
                    }
                ),
                encoding="utf-8",
            )

            data = bytearray(0x100000)
            base = 0x005FF000
            offset = base - 0x00500000
            data[offset + 0x0C : offset + 0x10] = (0x12345678).to_bytes(4, "little")
            data[offset + 0x158 : offset + 0x15A] = (0x0700).to_bytes(2, "little")
            work_ram.write_bytes(data)

            scenario = {
                "registers": {"g13": "0x005ff000"},
                "expect": {
                    "cpu": {
                        "local_frame_depth": 0,
                        "procedure_calls": 1,
                    },
                    "probe": {
                        "exploratory": False,
                    },
                    "work_ram": [
                        {
                            "base": "g13",
                            "offset": "0x0c",
                            "size": 4,
                            "value": "collision",
                        },
                        {
                            "base": "g13",
                            "offset": "0x158",
                            "size": 2,
                            "value": "0x0700",
                        },
                    ],
                },
            }

            errors = run_scenario.verify_expectations(
                scenario,
                {"collision": 0x12345678},
                state,
                work_ram,
            )
            self.assertEqual(errors, [])

    def test_mismatch_is_reported(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            state = root / "state.json"
            work_ram = root / "workram.bin"

            state.write_text(
                json.dumps({"cpu": {"local_frame_depth": 1}, "probe": {}}),
                encoding="utf-8",
            )
            work_ram.write_bytes(bytes(0x100000))

            scenario = {
                "registers": {"g13": "0x005ff000"},
                "expect": {
                    "cpu": {"local_frame_depth": 0},
                    "work_ram": [
                        {
                            "base": "g13",
                            "offset": 0,
                            "size": 4,
                            "value": "0x1",
                        }
                    ],
                },
            }

            errors = run_scenario.verify_expectations(
                scenario,
                {},
                state,
                work_ram,
            )
            self.assertEqual(len(errors), 2)
            self.assertTrue(any("cpu.local_frame_depth" in error for error in errors))
            self.assertTrue(any("work_ram" in error for error in errors))


if __name__ == "__main__":
    unittest.main()
