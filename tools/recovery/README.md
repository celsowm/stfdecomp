# Recovery tooling

This directory contains evidence-first recovery helpers for the STF fork.
They do not embed Sonic the Fighters ROM data.

## Cross-title matcher

Compare recovered VF2 C regions with STF assembly by normalized Model 2
addresses rather than function names or code addresses:

    python tools/recovery/model2_crossmatch.py \
        --vf2-root ../vf2-decomp \
        --stf-root . \
        --min-shared 2 \
        --top 5 \
        --output out/vf2-stf-crossmatch.md

JSON output is also available with --format json.

Cross-title matches are hypotheses. They do not automatically rename STF
functions or prove device semantics.

## Neutral i960 execution core

The hardware-neutral decoder/executor lives in:

    tools/recovery/i960/

It is derived from the independently validated celsowm/vf2-decomp CPU recovery
code under its retained BSD-3-Clause license. The executor no longer depends on
vf2_model2a; every memory access crosses stf_i960_bus.

Build and test:

    make recovery-test

The default Model 2B adapter maps read-only ROM/data and only raw storage whose
role is well-supported. GEO/TGP/SHARC, controls, IRQ/timers, I/O/serial and
other devices remain fail-closed.

See:

    docs/MODEL2B_RECOVERY_RUNTIME.md
    docs/MODEL2B_HARDWARE_EVIDENCE.md

## Corridors and scenarios

Generate a symbol map:

    make symbols

Run a named function:

    python tools/recovery/run_corridor.py \
        --symbols build/rom_code1.nm \
        --entry camera_init \
        --stack 0x005ff800

When locally extracted files exist, the launcher automatically attaches:

    rom/rom_data.bin
    rom/rom_ep.bin

Reproducible scenario files are handled by:

    python tools/recovery/run_scenario.py SCENARIO.json

Task scenarios may name descriptors such as fa_camera or fa_coli; the init
entry is extracted mechanically from the STF task table.

## Object/fighter field inference

Given JSONL memory traces, rank repeated offsets relative to known object bases:

    python tools/recovery/trace_fields.py TRACE.jsonl \
        --base fighter0=ADDRESS \
        --base fighter1=ADDRESS \
        --window 0x2000 \
        --json out/fighter-fields.json

Use STF-derived bases. VF2 offsets are methodology/evidence, not STF layout
truth.

## TGP/geometry traffic classification

Classify observed traffic:

    python tools/recovery/classify_tgp_trace.py TRACE.jsonl \
        --json out/tgp-report.json

STF itself establishes the 0x00880000 function path and 0x00884000 FIFO path.
The classifier may recognize those addresses, but the default runtime still
does not synthesize TGP/SHARC responses.

## First-fault aggregation

Combine multiple recovery runs:

    python tools/recovery/summarize_faults.py out/recovery/*.jsonl \
        --markdown out/recovery/faults.md

This helps choose the next device behavior by measured blocker frequency.

## Differential state

Compare a recovered snapshot with a controlled reference:

    python tools/recovery/compare_state.py reference.json recovered.json

## Evidence rules

- Cross-title similarity is evidence, not proof.
- Unknown branches and devices stay explicit.
- Prefer controlled traces and differential comparisons.
- Keep provisional structure fields neutral by offset.
- Probe allowances make a run exploratory.
- Do not commit ROMs, reconstructed ROM regions, extracted proprietary assets,
  or large raw traces.


## Sonic Championship ROM-backed secondary check

A local `schamp.zip` can be used as secondary evidence without committing ROM
bytes:

    python tools/recovery/validate_schamp_rom.py /path/to/schamp.zip

The validator checks the CRCs of the schamp asset ROMs used by the repository,
interleaves `epr-19141.15` + `epr-19142.16` in memory, and verifies the
collision-corridor signature observed in Sonic Championship:

- `coli_init` at `0x000293B8`;
- embedded `collision` address `0x00029418`;
- recovered initialization constants at their expected offsets.

This is intentionally marked **secondary evidence**. The reference STF i960
program remains the `sfight` set (`epr-19001.15` + `epr-19002.16`).
