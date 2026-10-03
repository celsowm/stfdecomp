# Neutral i960 recovery core

This directory contains the host-side Intel i960 recovery core used by the STF
fork. It is deliberately separate from the original ROM rebuild Makefile.

The decoder and executor originated from the independently validated i960
recovery work in celsowm/vf2-decomp. Its BSD-3-Clause notice is retained in
LICENSE.vf2-decomp. Hardware access has been refactored behind a generic bus.

## Architecture

    program ROM
        |
        v
    +-------------------+
    | i960 decoder      |
    | i960 executor     |
    | CPU/frame state   |
    +---------+---------+
              |
         stf_i960_bus
              |
              v
    +-------------------+
    | Model 2B adapter  |
    +----+---------+----+
         |         |
         |         +---- device/unknown -> callback or UNSUPPORTED
         |
         +-------------- read-only ROMs + proven raw storage

The CPU core does not know about VF2, Model 2A, TGP, SCSP, video, inputs or
arcade bookkeeping.

## Default Model 2B boundary

Implemented as data/storage only:

- program ROM at 0x00000000;
- 1 MiB work RAM at 0x00500000;
- buffer RAM at 0x00900000..0x0091ffff;
- CPU-control storage at 0x00e00000..0x00e00037;
- palette and color-translation storage;
- backup RAM, initialized to 0xff;
- texture RAM 0/1 and luma RAM;
- optional read-only rom_data.bin at 0x02000000;
- optional read-only rom_ep.bin at 0x03000000 and 0x06000000.

Fail-closed by default:

- GEO and geometry-program windows;
- TGP/SHARC function/FIFO/IOP paths;
- copro/GEO/video controls;
- IRQ/timers;
- tile/video behavior;
- I/O/serial;
- render-mode behavior;
- sound-board communication.

This distinction prevents byte storage from being mistaken for recovered device
semantics.

## Build and test

No game ROM is needed:

    make recovery-test

Equivalent CMake commands:

    cmake -S tools/recovery/i960 -B build/recovery-i960 -DCMAKE_BUILD_TYPE=Release
    cmake --build build/recovery-i960 --config Release
    ctest --test-dir build/recovery-i960 -C Release --output-on-failure

GitHub Actions runs the same recovery suite on Ubuntu and Windows.

## Corridor runners

The build produces:

    stf_i960_corridor
    stf_i960_probe

Use stf_i960_corridor for reproducible corridors, register seeds, work-RAM
preload/dump, JSONL traces, final state and explicit exploratory probe policies.

Example:

    build/recovery-i960/stf_i960_corridor \
        --rom rom/rom_code1.bin \
        --data-rom rom/rom_data.bin \
        --ep-rom rom/rom_ep.bin \
        --entry 0x00000000 \
        --steps 10000 \
        --trace out/boot.jsonl \
        --state out/boot-state.json

The symbol-aware wrapper is usually easier:

    make symbols

    python tools/recovery/run_corridor.py \
        --symbols build/rom_code1.nm \
        --entry camera_init \
        --stack 0x005ff800

It automatically attaches conventional rom_data.bin/rom_ep.bin files when they
exist.

For repeatable experiments:

    python tools/recovery/run_scenario.py tools/recovery/scenarios/my-run.json

Use stf_i960_probe for small experiments requiring explicit write seeds and
watched memory ranges.

## Probe policy

Unknown hardware is not silently zero-filled.

A specifically measured write can be admitted temporarily:

    --allow-write START:END

A measured read can be replayed:

    --stub-read ADDRESS=VALUE

If a run consumes either policy, it is exploratory rather than reference
evidence.

## Bus contract

The CPU receives stf_i960_bus with:

- program_image/program_size for instruction decoding;
- read/write callbacks;
- opaque adapter context;
- trace callback and synchronized step id.

The executor therefore preserves broad VF2-proven i960 instruction coverage
without depending on vf2_model2a.

## Recovery rule

A successfully executed CPU instruction says nothing about whether a touched
Model 2B device is understood. Unsupported device accesses must remain explicit
until STF-specific evidence establishes the behavior.

See:

    docs/MODEL2B_HARDWARE_EVIDENCE.md
    docs/MODEL2B_RECOVERY_RUNTIME.md
