import json
import tempfile
import unittest
from pathlib import Path

from decomp_progress import scan_asm, union_size, audit


class TestProgress(unittest.TestCase):
    def test_label_vs_data_and_local(self):
        with tempfile.TemporaryDirectory() as temp:
            p = Path(temp) / "rom_code1.s"
            p.write_text(
                ".text\nentry:\n call foo\nsub_000010:\n bal entry\n"
                "loc_000014:\n b loc_000014\nfoo:\n ret\n"
                ".data\nword_44:\n .long 1\n", encoding="utf-8"
            )
            labels, calls, lines = scan_asm(p)
            self.assertEqual(calls, {"foo", "entry"})
            self.assertEqual(labels["sub_000010"]["address"], 16)
            self.assertEqual(labels["word_44"]["section"], "data")
            self.assertEqual(lines, 12)

    def test_union_not_double_counted(self):
        self.assertEqual(
            union_size([(10, 20), (15, 30), (10, 20), (40, 50)]),
            30,
        )

    def make_root(self, temp):
        root = Path(temp)
        (root / "src/asm").mkdir(parents=True)
        (root / "src/recovered").mkdir()
        (root / "src/recovered/hit.c").write_text("int test = 1;\n")
        for name in ("rom_code1.s", "rom_code2.s",
                     "rom_code3.s", "rom_code4.s"):
            (root / "src/asm" / name).write_text(
                ".text\nentry:\n call sub_000010\nsub_000010:\n ret\n"
                "loc_000014:\n ret\n.data\nword_44:\n .long 1\n",
                encoding="utf-8"
            )
        return root

    def test_partial_counts_are_not_completion(self):
        with tempfile.TemporaryDirectory() as temp:
            root = self.make_root(temp)
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"schema_version": 1, "corridors": [
                {"id": "a", "start": "0x10", "end": "0x20",
                 "status": "partial", "owner_symbols": ["sub_000010"],
                 "c_sources": ["src/recovered/hit.c"],
                 "reference": "manifest.json"},
                {"id": "b", "start": "0x18", "end": "0x28",
                 "status": "partial", "c_sources": ["src/recovered/hit.c"],
                 "reference": "manifest.json"}
            ]}))
            result = audit(root, manifest)
            self.assertEqual(result["counts"]["candidate_functions"], 1)
            self.assertEqual(result["counts"]["candidate_functions_with_partial_C"], 1)
            self.assertEqual(result["counts"]["unique_corridor_rom_bytes"], 24)
            self.assertEqual(result["counts"]["byte_exact_complete_functions"], 0)
            self.assertIsNone(result["percentages"]["actual_full_game_decomp"])

    def test_missing_source_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = self.make_root(temp)
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"schema_version": 1, "corridors": [
                {"id": "bad", "start": "0x10", "end": "0x20",
                 "status": "partial", "c_sources": ["src/recovered/missing.c"],
                 "reference": "manifest.json"}
            ]}))
            with self.assertRaises(ValueError):
                audit(root, manifest)

    def test_unproved_complete_status_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            root = self.make_root(temp)
            manifest = root / "manifest.json"
            manifest.write_text(json.dumps({"schema_version": 1, "corridors": [
                {"id": "bad", "start": "0x10", "end": "0x20",
                 "status": "byte_exact", "c_sources": ["src/recovered/hit.c"],
                 "reference": "manifest.json"}
            ]}))
            with self.assertRaises(ValueError):
                audit(root, manifest)


if __name__ == "__main__":
    unittest.main()
