# Sonic the Fighters — set_obj CPU recovery

This document tracks the STF-native set_obj corridor; it is NOT a port of
VF2 set_obj. The original STF Assembly excerpt is reproducible with:

    python tools/recovery/set_obj_evidence.py --out-dir build/set-obj

The source is src/asm/rom_code1.s:4910 through 4965, with related
internal labels. The function shares code with other entrypoints,
so those source lines are not a certified ROM byte range.

| Entry | ASM line | Direct callsites | State |
| --- | ---: | ---: | --- |
| set_obj | 4910 | 467 | CPU semantic adapter recovered |
| set_obj_tpd | 5012 | 7 | Not yet translated |
| set_obj_thd | 5059 | 22 | Not yet translated |

## Implemented CPU-visible behavior

src/recovered/set_obj_runtime.h and .c provide the opt-in
stf_set_obj_apply_model2 normal-entry adapter:

1. Read POLYGON_LIMIT at 0x501018 and POLYGON at 0x50101C. If polygon
   exceeds limit, return without any device operations.
2. Use bit 15 of g0 to choose the normal table starting at
   POLYGON_NUM_OFFSET (0x020E0004, stride 16), or the alternate table
   at 0x0647F004, masked index and max at 0x0647F000.
   Reject out-of-range indices and records whose first word is zero.
   The full poly_data_err behavior is NOT yet recovered.
3. Issue three FIFO writes of 0x09801313 and one FIFO read;
   save g0 as mae_poly; increment 0x501010.
4. Issue 0x3C007878 to FIFO, read GEO+0x2008, then write
   that pointer + 0x48 to GEO+0x1008.
5. Queue the retrieved pointer, zero g1, the record quad (four
   words), P2_POLYGON and POLYGON. Receive and apply the final
   two FIFO replies to P2_POLYGON and POLYGON, in that order.

The host callback handles each GEO/FIFO operation and provides real
replies or rejects the operation. The implementation does not model
unknown Model 2B hardware. The 17-event order is covered by synthetic
C tests, including failure after every individual operation and the
normal and bit-15 record tables. CPU/device writes issued before a
callback failure are not rolled back.

A separate source-order Python test compares opcode and control-flow
markers against the original disassembly, so accidental copying of
VF2's protocol is detected in CI.

## Do NOT equate VF2 with STF

VF2 set_obj (0x7C60) was separately recovered and oracle-tested.
It uses 0x1A003434 and computes polygon changes from the selected
record. The STF normal set_obj instead issues 0x09801313 three times,
issues 0x3C007878, and receives polygon values from FIFO replies.
0x1A003434 exists in STF as the separate set_obj_tpd and set_obj_thd
paths. A shared name or global address is not proof of same behavior.

## Still pending

- Real STF hardware FIFO replies, timing and graphics submission.
- Poly_data_err side effects, TPD, THD, common/go, save/load/same.
- Register/condition-code and instruction-exact differential replay.
- Linking the recovered C into the actual ROM rebuild.
- Authoritative STF ROM byte boundaries and independently measured
  function-level correctness. This feature does not change the
  global complete-decompilation percentage (still unknown).

No proprietary ROMs or extracted assets are committed.
