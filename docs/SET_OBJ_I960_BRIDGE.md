# STF object submission — i960 call-boundary integration

This milestone integrates recovered object-submission CPU semantics with the
existing i960 executor and Model 2B bus. It does NOT intercept or replace
original STF ROM instructions, and is not proof of instruction or visual
equivalence. No original ROM or hardware oracle was supplied.

## Build and run

~~~sh
cmake -S tools/recovery/i960 -B build/recovery-i960
cmake --build build/recovery-i960 -j
ctest --test-dir build/recovery-i960 --output-on-failure
~~~

The CTest target is stf_set_obj_i960_bridge_tests and the static library is
stf_set_obj_i960_bridge. The regression suite executes an actual decoded
i960 instruction from a small synthetic program before invoking the
call-boundary adapter with that CPU object. This tests the integration
interface, not the complete original ROM routine.

## Supported entrypoints and state

The function stf_set_obj_i960_bridge_execute(cpu, hardware, entry, result)
supports all nine documented CPU-level paths:

| Entry | Behavior |
| --- | --- |
| NORMAL | set_obj normal FIFO / GEO protocol |
| TPD | set_obj_tpd quad word 0 override from g2 |
| THD | set_obj_thd quad word 1 override from g2 |
| COMMON | set_obj_common record and polygon update |
| SAME | set_obj_same strict polygon-limit gate |
| MIRROR | set_obj_mir alternate mirror-mode GEO operations |
| FIFO | set_obj_fifo three FIFO quads, scratch, conditional common/THD tail |
| SAVE | set_obj_save three FIFO quads into caller g2 buffer |
| LOAD | set_obj_load three saved caller g2 quads into GEO |

The bridge binds original i960 global registers as follows: g0 (register
16) is the object selector; g2 (18) is a selector-specific override,
FIFO argument, or caller buffer pointer; g3 (19) is the FIFO scratch
pointer; g7 (23) contains conditional flags; g10 (26) must equal
0x00800000, g11 (27) must equal 0x00880000, and g12 (28) must equal
0x00004000. Different GEO/FIFO register mappings fail closed.

It reads POLYGON_LIMIT at 0x501018, POLYGON at 0x50101C, P2_POLYGON
at 0x5010D0, polygon counter at 0x501010, and mae_poly at 0x50FFFC
from the actual i960 bus. Descriptor tables at 0x020E0000 and
0x0647F000 are also fetched from attached ROM or device callbacks.
Missing ROM mapping fails closed. The existing Model 2B bus is used for
every GEO/FIFO access and can reject unsupported hardware operations.

FIFO and SAVE/LOAD bind g3 and g2 buffers through word-aligned
48-byte windows of work RAM. The recovered adapter reports how
many quads were accepted, saved and advanced. The CPU register snapshot
itself is NOT mutated: caller-buffer saves and final global stores
are committed after the C routine returns, whereas the original i960
instruction stream interleaves those effects with other operations.
This limitation must be accounted for during exact differential checks.

## Device-only trace comparison

The stf_set_obj_i960_mmio_capture API filters the existing i960 bus
trace to GEO and FIFO windows. The comparison API checks event order,
full addresses, read/write kinds, transfer sizes, bus statuses, exact
write/read bytes, and event count. It reports the zero-based first
difference. Capture overflow is explicitly invalid, not a match.

An original-CPU-vs-C differential experiment requires two independently
initialized runs with the same ROM, entry state and deterministic GEO/
FIFO responses. Feed each run to the event collector, compare device
events, then separately compare final CPU registers, arithmetic control,
RAM and the interrupt state. The current CI uses synthetic fixtures,
tests a modified read reply and a missing event, and rejects truncated
captures. It has not measured agreement against a real STF ROM trace.

## Critical gaps

1. No completed original STF i960 ROM call using verified real device
   responses; only the synthetic integration instruction has run.
2. CPU register, condition-code and scheduler effects are not recreated.
3. The bridge prefetches descriptors and commits caller-buffer/global RAM
   updates at the boundary rather than in exact instruction order.
4. Original 128-bit ldq/stq device transfers use four ordered 32-bit
   callback operations instead of one verified atomic bus transaction.
5. poly_data_err still reports failure; debug display, original interrupt
   polling and actual coprocessor behavior are not simulated.
6. Recovered C is not yet linked into the original ROM build; full-game
   decompilation percentage is still unknown.

For a genuine 1:1 milestone, the next experiment must run and capture
the original STF routine under the i960 executor using original
ROM evidence and a documented non-fabricated device fixture.
