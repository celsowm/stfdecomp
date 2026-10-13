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
| set_obj_tpd | 5012 | 7 | CPU semantic adapter recovered |
| set_obj_thd | 5059 | 22 | CPU semantic adapter recovered |

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

## Recovered TPD/THD paths

The new stf_set_obj_special_apply_model2() exposes the two distinct
original entrypoints. Both first apply the polygon gate. On the open
path they:

1. Write 0xB0B to GEO+0xB0; send 0x1A003434 to the FIFO.
2. Read the GEO pointer at +0x2008, write pointer+0x30 to +0x1008,
   write the pointer to FIFO and consume one FIFO response.
3. Resolve the same normal/bit-15 polygon tables. Crucially, this
   validation happens AFTER the first six device events (unlike
   regular set_obj). Invalid entries are rejected without pretending
   to execute the unrecovered poly_data_err handler.
4. Increment the polygon counter; add the lower 16 bits of the
   record's fourth word to POLYGON. Overwrite the fourth record word
   with 0xFFFFFFFF.
5. set_obj_tpd overrides record word 0 with g2, whereas set_obj_thd
   overrides word 1 with g2. The other source words are preserved.
6. Write 0x101 at GEO+0x10, then submit the four-word record at
   GEO+g12. The original i960 stq quad store is exposed as four
   ordered portable callback writes; atomic GEO bus behavior is
   not claimed.

CTest verifies both variants against normal and bit-15 record sets,
the exact eleven events, selector rejection after the initial FIFO
operations, and every callback failure boundary. The source-order
Python regression also guards the original TPD/THD opcode and branch
into set_obj_go.

## Do NOT equate VF2 with STF

VF2 set_obj (0x7C60) was separately recovered and oracle-tested.
It uses 0x1A003434 and computes polygon changes from the selected
record. The STF normal set_obj instead issues 0x09801313 three times,
issues 0x3C007878, and receives polygon values from FIFO replies.
0x1A003434 exists in STF as the separate set_obj_tpd and set_obj_thd
paths. A shared name or global address is not proof of same behavior.

## Still pending

- Real STF hardware FIFO replies, timing and graphics submission.
- Poly_data_err side effects, common/go shared entry semantics beyond the
  TPD/THD paths, save/load/same and additional display variants.
- Register/condition-code and instruction-exact differential replay.
- Linking the recovered C into the actual ROM rebuild.
- Authoritative STF ROM byte boundaries and independently measured
  function-level correctness. This feature does not change the
  global complete-decompilation percentage (still unknown).

No proprietary ROMs or extracted assets are committed.
