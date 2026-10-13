import tempfile
import unittest
from pathlib import Path

from decomp_triage import build_triage, references, to_md


class TestTriage(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        (self.root / "src/asm").mkdir(parents=True)
        (self.root / "src/recovered").mkdir()
        for name in ("rom_code1.s", "rom_code2.s", "rom_code3.s", "rom_code4.s"):
            (self.root / "src/asm" / name).write_text("", encoding="utf-8")
        (self.root / "src/asm/rom_code1.s").write_text(
            ".text\n"
            "sub_000010:\n  call fighter_update\n  bal fighter_update\n"
            "sub_000020:\n  call sub_000010 # call decoy\n"
            "fighter_update:\n  ret\n"
            "pointer_only:\n  ret\n"
            "loc_000050:\n  ret\n"
            ".data\n"
            "task_table:\n .long pointer_only\n"
            " .long loc_000050\n"
            " .long 0x1234 # .long fighter_update\n"
            " .long pointer_only+4\n",
            encoding="utf-8",
        )
        (self.root / "src/recovered/fighter_update.c").write_text(
            "int not_verified(void) { return 1; }\n", encoding="utf-8"
        )

    def tearDown(self):
        self.temp.cleanup()

    def progress(self):
        def cand(name, addr, mapped=False):
            return {
                "symbol": name, "file": "rom_code1.s", "line": 1,
                "address": addr,
                "address_basis": "numeric_label" if addr is not None else None,
                "mapped_evidence_ids": ["synthetic"] if mapped else [],
            }
        return {
            "function_candidates": [
                cand("sub_000010", 16),
                cand("sub_000020", 32),
                cand("fighter_update", None),
            ]
        }

    def test_calls_and_pointer_table_are_separate(self):
        targets = {
            "sub_000010", "sub_000020", "fighter_update",
            "pointer_only", "loc_000050",
        }
        calls, ptrs, where = references(self.root, targets)
        self.assertEqual(calls["fighter_update"], 2)
        self.assertEqual(calls["sub_000010"], 1)
        self.assertEqual(calls["decoy"], 0)
        self.assertEqual(ptrs["pointer_only"], 2)
        self.assertEqual(ptrs["loc_000050"], 1)
        self.assertEqual(ptrs["fighter_update"], 0)
        self.assertEqual(where["pointer_only"], ["rom_code1.s", 12])

    def test_prio_and_candidate_next_start_is_not_function_end(self):
        triage = build_triage(self.root, self.progress())
        self.assertEqual(triage["stats"]["c_source_files"], 1)
        self.assertEqual(triage["stats"]["prioritized_candidates"], 3)
        self.assertEqual(triage["stats"]["unverified_pointer_target_names"], 1)
        self.assertEqual(
            triage["stats"]["unmapped_candidates_with_unverified_C_filename_hints"], 1
        )
        rows = {r["symbol"]: r for r in triage["priority_queue"]}
        self.assertEqual(rows["sub_000010"]["next_known_candidate_start"], 32)
        self.assertIsNone(rows["sub_000020"]["next_known_candidate_start"])
        self.assertEqual(rows["fighter_update"]["direct_callsites"], 2)
        self.assertEqual(
            rows["fighter_update"]["c_filename_hints_unverified"],
            ["src/recovered/fighter_update.c"],
        )
        self.assertFalse(rows["fighter_update"]["mapped_partial_C"])
        self.assertEqual(
            triage["pointer_entry_hypotheses"][0]["symbol"], "pointer_only"
        )
        self.assertNotIn("loc_000050", [
            r["symbol"] for r in triage["pointer_entry_hypotheses"]
        ])
        self.assertIn("NOT a decompilation percentage", to_md(triage))

    def test_mapped_entry_skipped_from_priority_top(self):
        p = self.progress()
        p["function_candidates"][2]["mapped_evidence_ids"] = ["fixture"]
        triage = build_triage(self.root, p)
        self.assertTrue(
            next(x for x in triage["priority_queue"]
                 if x["symbol"] == "fighter_update")["mapped_partial_C"]
        )
        self.assertNotIn("| fighter_update |", to_md(triage).split("## Areas")[0])


if __name__ == "__main__":
    unittest.main()
