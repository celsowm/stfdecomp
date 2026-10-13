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

    def test_callsites_are_grounded(self):
        self.assertEqual(len(self.evidence["calls"]["set_obj"]), 467)
        self.assertEqual(len(self.evidence["calls"]["set_obj_tpd"]), 7)
        self.assertEqual(len(self.evidence["calls"]["set_obj_thd"]), 22)


if __name__ == "__main__":
    unittest.main()
