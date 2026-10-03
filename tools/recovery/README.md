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

The default ports match the current Model 2 research baseline. Override them
when Model 2B traces show different addresses.

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

The first STF Model 2B adapter implements only:

- program ROM at `0x00000000`;
- the 1 MiB work-RAM window at `0x00500000`;
- explicit callbacks for device ranges;
- fail-closed `STF_ERROR_UNSUPPORTED` for unmodeled hardware.

See `i960/README.md` and `../../docs/MODEL2B_RECOVERY_RUNTIME.md`.

Build the ROM-independent host tests with:

    cmake -S tools/recovery/i960 -B build/recovery-i960 -DCMAKE_BUILD_TYPE=Release
    cmake --build build/recovery-i960 --config Release
    ctest --test-dir build/recovery-i960 -C Release --output-on-failure
