# Recovery tooling

This directory contains title-agnostic recovery helpers added in the fork.
They do not require or embed Sonic the Fighters ROM data.

## VF2 <-> STF cross-title matcher

Compare recovered VF2 C regions with STF assembly regions by normalized Model 2
addresses rather than function names or code addresses:

    python tools/recovery/model2_crossmatch.py \
        --vf2-root ../vf2-decomp \
        --stf-root . \
        --min-shared 2 \
        --top 5 \
        --output out/vf2-stf-crossmatch.md

Machine-readable output:

    python tools/recovery/model2_crossmatch.py \
        --vf2-root ../vf2-decomp \
        --stf-root . \
        --format json \
        --output out/vf2-stf-crossmatch.json

Run its ROM-independent tests with:

    python tools/recovery/test_model2_crossmatch.py

## Object/fighter field inference

Given JSONL memory traces, rank repeated offsets relative to one or more object
bases:

    python tools/recovery/trace_fields.py trace.jsonl \
        --base fighter0=0x00510980 \
        --base fighter1=0x00512980 \
        --window 0x2000 \
        --json out/fighter-fields.json

The addresses above are only an example from VF2 methodology. For STF, obtain
the actual bases from evidence before using them.

## TGP/geometry traffic classification

Classify FIFO command classes, geometry writes and coprocessor function-port
traffic:

    python tools/recovery/classify_tgp_trace.py trace.jsonl \
        --json out/tgp-report.json

The defaults now come from STF's own linker map: geometry RAM
`0x00800000..0x00803fff` and declared coprocessor/control bases at
`0x008c0000`, `0x00980000`, `0x00980008`, and `0x00980014`.
No VF2-only FIFO address is assumed; pass `--fifo` only after STF evidence
establishes it.

## Evidence rules

- Cross-title matches are hypotheses, not semantic proof.
- Unknown branches remain unknown.
- Prefer controlled traces and differential state comparisons.
- Keep provisional structure fields neutrally named by offset.
- Do not commit ROMs, reconstructed ROM regions or extracted proprietary
  assets.


## Neutral i960 execution core

The fork now includes a hardware-neutral host-side i960 decoder/executor in:

    tools/recovery/i960/

It is derived from the independently validated `celsowm/vf2-decomp` CPU
recovery code under its retained BSD-3-Clause license, but the executor no
longer depends on `vf2_model2a`. Every memory access crosses `stf_i960_bus`.

The STF Model 2B adapter currently implements:

- program ROM at `0x00000000`;
- the 1 MiB work-RAM window at `0x00500000`;
- raw geometry RAM at `0x00800000..0x00803fff`;
- the four declared buffer-RAM banks at `0x00900000..0x0091ffff`;
- named linker-map hints for coprocessor, IRQ, timer, video and I/O faults;
- explicit callbacks for device ranges;
- fail-closed `STF_ERROR_UNSUPPORTED` for unmodeled hardware.

See `i960/README.md` and `../../docs/MODEL2B_RECOVERY_RUNTIME.md`.

Build the ROM-independent host tests with:

    cmake -S tools/recovery/i960 -B build/recovery-i960 -DCMAKE_BUILD_TYPE=Release
    cmake --build build/recovery-i960 --config Release
    ctest --test-dir build/recovery-i960 -C Release --output-on-failure
