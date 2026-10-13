"""Guard the recovered set_obj semantics against the committed STF ASM."""
import unittest
from pathlib import Path
from set_obj_evidence import analyze

ROOT = Path(__file__).resolve().parents[2]


class SetObjEvidence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.evidence = analyze(ROOT, span=140)

    def window(self, symbol):
        found = [r for r in self.evidence["routines"] if r["name"] == symbol]
        self.assertEqual(len(found), 1)
        return "\n".join(
            r["text"].split("#", 1)[0].strip()
            for r in found[0]["excerpt"]
        )

    def test_normal_opcodes_and_order(self):
        body = self.window("set_obj")
        expected = [
            "mov     0, g1", "POLYGON_LIMIT, r4",
            "POLYGON, r5", "cmpobg  r5, r4",
            "bbs     0xF, r15", "0x647F004[r15*16]",
            "0x9801313", "mae_poly", "0x501010",
            "0x3C007878", "0x2008(g10)", "0x1008(g10)",
            "P2_POLYGON, r15", "st      r15, P2_POLYGON",
            "st      r15, POLYGON", "ret",
        ]
        pos = -1
        for token in expected:
            next_pos = body.find(token, pos + 1)
            self.assertGreater(next_pos, pos, token)
            pos = next_pos
        self.assertNotIn("0x1A003434", body.split("set_obj_tpd:", 1)[0])

    def test_tpd_thd_not_same_protocol(self):
        for symbol in ("set_obj_tpd", "set_obj_thd"):
            body = self.window(symbol)
            self.assertIn("0x1A003434", body)
            self.assertIn("0x2008(g10)", body)
            self.assertIn("0x1008(g10)", body)
            self.assertIn("set_obj_go", body)

    def test_common_and_same_are_independent_entries(self):
        common = self.window("set_obj_common")
        for mark in (
            "0x501010", "ldq     (g0), r8", "set_obj_go:",
            "shro    16, r11, r3", "0xFFFF, r4",
            "POLYGON", "subo    1, 0, r11",
            "0x101, g14", "stq     r8, (g10)[g12*1]"
        ):
            self.assertIn(mark, common)
        same = self.window("set_obj_same")
        self.assertIn("cmpobl  r5, r4, set_obj_common", same)
        self.assertIn("ret", same)

    def test_save_and_load_original_quad_order(self):
        saved = self.window("set_obj_save")
        loaded = self.window("set_obj_load")
        self.assertIn("0x2800505, r15", saved)
        # The save window includes the later load routine, so isolate
        # original body up to the source label for set_obj_load.
        save_body = saved.split("set_obj_load:", 1)[0]
        self.assertEqual(save_body.count("ldq     (g11)[g12*1], r12"), 3)
        self.assertEqual(save_body.count("addo    0x10, g2, g2"), 3)
        self.assertEqual(save_body.count("stq     r12, (g2)"), 3)
        self.assertEqual(loaded.split("set_obj_same:", 1)[0].count(
            "ldq     (g2), r12"), 3)
        self.assertEqual(loaded.split("set_obj_same:", 1)[0].count(
            "stq     r12, (g10)[g12*1]"), 3)
        self.assertIn("b       set_obj_common", save_body)
        self.assertIn("b       set_obj_common",
                      loaded.split("set_obj_same:", 1)[0])

    def test_polygon_error_is_debug_wait_not_normal_return(self):
        body = self.window("poly_data_err")
        self.assertIn("un_poly_data_err_flag", body)
        self.assertIn("debug_disp_int", body)
        self.assertIn("INTERUPT_FLAGS_HELD", body)
        self.assertIn("bbc     4, r8", body)
        self.assertIn("bbc     13, r8", body)

    def test_callsites_are_grounded(self):
        self.assertEqual(len(self.evidence["calls"]["set_obj"]), 467)
        self.assertEqual(len(self.evidence["calls"]["set_obj_tpd"]), 7)
        self.assertEqual(len(self.evidence["calls"]["set_obj_thd"]), 22)


if __name__ == "__main__":
    unittest.main()
