#!/usr/bin/env python3

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).with_name("model2_crossmatch.py")
SPEC = importlib.util.spec_from_file_location("model2_crossmatch", MODULE_PATH)
MODULE = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = MODULE
SPEC.loader.exec_module(MODULE)


class CrossmatchTests(unittest.TestCase):
    def test_symbol_resolution_and_matching(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            vf2 = root / "vf2-decomp"
            stf = root / "stfdecomp"

            (vf2 / "src" / "recovered").mkdir(parents=True)
            (vf2 / "src" / "hardware").mkdir(parents=True)
            (stf / "src" / "asm").mkdir(parents=True)
            (stf / "src" / "lib").mkdir(parents=True)

            (vf2 / "README.md").write_text("vf2", encoding="utf-8")
            (vf2 / "src" / "recovered" / "geometry.c").write_text(
                """
enum {
    VF2_LIMIT = 0x00501018u,
    VF2_TABLE = 0x020E0004u,
    VF2_FOCUS = 0x00501084u
};

int recovered_submit(void) {
    return VF2_LIMIT + VF2_TABLE + VF2_FOCUS;
}
""",
                encoding="utf-8",
            )

            (stf / "src" / "lib" / "rom.ld").write_text(
                """
SECTIONS {
    POLYGON_LIMIT = 0x501018;
    POLYGON_NUM_OFFSET = 0x20E0004;
    focus_dist_x = 0x501084;
}
""",
                encoding="utf-8",
            )
            (stf / "src" / "asm" / "rom_code1.s").write_text(
                """
candidate_submit:
    ld POLYGON_LIMIT, r4
    ld POLYGON_NUM_OFFSET, r5
    ld focus_dist_x, r6
unrelated:
    lda 0x500024, r4
""",
                encoding="utf-8",
            )

            vf2_regions = MODULE.collect_vf2_regions(vf2)
            stf_regions = MODULE.collect_stf_regions(stf)
            matches = MODULE.score_matches(vf2_regions, stf_regions, 2, 5)

            self.assertEqual(len(vf2_regions), 1)
            self.assertGreaterEqual(len(stf_regions), 1)
            self.assertEqual(matches[0].stf.name, "candidate_submit")
            self.assertEqual(len(matches[0].shared), 3)

    def test_known_anchor_report_resolves_stf_names(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            (root / "src" / "lib").mkdir(parents=True)
            (root / "src" / "lib" / "rom.ld").write_text(
                "POLYGON_LIMIT = 0x501018;\n",
                encoding="utf-8",
            )

            report = MODULE.anchor_report(root)
            item = next(row for row in report if row["address"] == 0x00501018)
            self.assertIn("POLYGON_LIMIT", item["stf_symbols"])


if __name__ == "__main__":
    unittest.main()
