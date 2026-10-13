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

## Additional recovered source-level pathways: common, same, save, load

The new src/recovered/set_obj_state_runtime.h and .c support four
original STF CPU entrypoints, independent of the normal and TPD/THD ones:

| Original entry | Semantics now represented |
| --- | --- |
| set_obj_common (rom_code1.s:4967) | Validate the normal/extended object record, increment 0x501010, add record fourth word low 16 bits to POLYGON, set word 3 to 0xFFFFFFFF and submit a 4-word quad to GEO after writing 0x101 to GEO+0x10 |
| set_obj_same (5152) | Branch to common only when POLYGON is **strictly less** than POLYGON_LIMIT; otherwise return without I/O |
| set_obj_save (5109) | Normal polygon gate; GEO+0xB0=0xB0B; FIFO opcode 0x02800505; read **three** 16-byte quads from FIFO, write them to GEO, save them into caller-owned g2 memory, and tail-branch to common |
| set_obj_load (5133) | Normal polygon gate; GEO+0xB0=0xB0B; read three saved quads from caller g2 memory, send to GEO, and tail-branch to common |

stf_set_obj_transfer_apply_model2() accepts twelve caller-owned 32-bit
words (3x16-byte quads) and reports the 48-byte g2 pointer progression.
Every read/write to the external FIFO and GEO is delegated to the host.
The i960 ldq/stq quad bus accesses are represented as four ordered word
callbacks, a portable approximation that **does not establish exact
hardware bus atomicity or cycle timing**. Successful completed quads
are saved and remain saved after later failures.

The new CTest exercises both common and same (including the important
difference between the strict SAME gate and the normal greater-than
gate), normal/extended polygon records, save→load roundtrip, event order
and failures in every bus position. The source-evidence Python suite
pins these same branches and three-quad counts to the original ASM.

## Original poly_data_err: now characterized, NOT yet executed

rom_code1.s:4818..4908 shows that this is **not** a simple error
return: it saves registers, invokes debug display helpers using
labels including "max poly", "err poly" and "mae poly",
restores registers and then spins on INTERUPT_FLAGS_HELD bits
4 and 13 before returning. We intentionally do **not** invent
equivalent frame scheduling, debug-display hardware or interrupt
inputs: table validation remains a fail-closed boundary until that
flow can be verified with a real STF oracle.

## New set_obj_fifo and set_obj_mir recovery

The CPU-visible branches at rom_code1.s:5158-5227 and 5229-5284
are implemented in src/recovered/set_obj_extended_runtime.c.
These are new semantic adapters, not ROM-linked replacements.

- set_obj_mir validates the normal or bit-15 polygon descriptor BEFORE
  reading the polygon-limit gate. On the open path it increments the
  polygon counter, adds the low 16 bits of the record's fourth word,
  configures GEO+0xB0=0xB0B, sends FIFO opcode 0x1A003434,
  writes GEO end-pointer=read-pointer+0x30, then GEO+0x110=0x1111
  and the modified four-word quad (fourth word set to 0xFFFFFFFF).
- set_obj_fifo consumes three 16-byte FIFO quads BEFORE checking
  caller g2, POLYGON_LIMIT or floating-point bit-pattern guards.
  It returns early after the FIFO reads when g2 == 1, POLYGON exceeds
  the limit, abs(first word) is below 0x3F800000, or abs(last word)
  exceeds 0x3E4CCCCD. Equality passes, using i960 ordinal comparison
  over sign-cleared float32 bit patterns.
- For accepted data, set_obj_fifo submits all three quads to GEO
  and copies each completed quad to caller-owned g3 scratch memory.
  Only THEN does it validate the polygon descriptor. On an invalid
  descriptor the original poly_data_err_fifo rewinds g3 by 48 bytes
  and enters the original debug/interrupt handler. The adapter
  reports this error and rewind without claiming to execute the
  handler.
- When (g7+0x1A4 bit 26) and unsigned (g7+0x1AA >= 4), the FIFO
  entry selects set_obj_thd_common with g2=0x00800000. Otherwise
  (g7+0x70C bits 0 and 2) and the same count condition select
  g2=0x00802000. All other cases use set_obj_common. The
  thd_common tail is shared with existing recovered code, and does
  NOT send a second FIFO setup opcode.

The new CTest checks normal/extended polygon tables, mirror command
ordering, three FIFO branch outcomes, four early gates, first/last
word equality, negative sign-bit cases, late-invalid-selector
behavior and all eleven mirror/thirty FIFO callback failure points.
The Python ASM evidence tests pin source ordering and poly_data_err_fifo.

Unverified: real Model 2B FIFO replies, i960 condition codes,
correct external quad-bus atomicity, geometry hardware timing and
original ROM rebuild equivalence.

## Still pending

- Real STF hardware FIFO replies, timing and graphics submission.
- Hardware-accurate poly_data_err debug display and interrupt polling,
  remaining less-common graphics entrypoints and coprocessor behavior.
- Register/CC effects of common/go/save/load, rather than only observable
  memory/FIFO/GEO state and explicit synthetic test callbacks.
- Register/condition-code and instruction-exact differential replay.
- Linking the recovered C into the actual ROM rebuild.
- Authoritative STF ROM byte boundaries and independently measured
  function-level correctness. This feature does not change the
  global complete-decompilation percentage (still unknown).

No proprietary ROMs or extracted assets are committed.

## Integration with existing i960 executor and Model 2B bus

See [the CPU snapshot / device trace bridge](SET_OBJ_I960_BRIDGE.md)
for normal, TPD, THD, common, same, mirror, FIFO and three-quad
save/load support, including the limitations of staged caller-memory
writes and synthetic-only differential tests.
