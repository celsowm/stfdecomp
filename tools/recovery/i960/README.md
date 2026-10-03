# Neutral i960 recovery core

This directory contains the host-side Intel i960 recovery core used by the STF
fork. It is deliberately separate from the original ROM rebuild Makefile.

The decoder and executor originated from the independently validated i960
recovery work in celsowm/vf2-decomp. The BSD-3-Clause notice is retained in
LICENSE.vf2-decomp. Hardware access has been refactored out of the CPU into a
generic bus interface.

## Architecture

    STF program image
           |
           v
    +-------------------+
    | i960 decoder      |
    | i960 executor     |
    | CPU/frame state   |
    +---------+---------+
              |
              | stf_i960_bus
              v
    +-------------------+
    | Model 2B adapter  |
    +----+---------+----+
         |         |
         |         +---- unknown/device ranges -> callback or UNSUPPORTED
         |
         +-------------- ROM + 0x00500000 work RAM

The CPU core does not know about VF2, Model 2A, TGP, SCSP, video, inputs or
arcade bookkeeping.

## Current Model 2B map

Implemented:

- program/data ROM starting at 0x00000000;
- 1 MiB work RAM starting at 0x00500000, matching the STF linker script;
- 16 KiB raw geometry RAM at 0x00800000..0x00803fff;
- 128 KiB raw buffer RAM at 0x00900000..0x0091ffff;
- named Model 2B hardware addresses derived from the STF linker map;
- extension callbacks for device-visible ranges;
- fail-closed behavior for unmapped reads/writes;
- ROM writes rejected.

Not modeled yet:

- TGP/coprocessor behavior and control registers;
- geometry program memory semantics;
- video/tile/palette/control windows;
- system/interrupt/timer registers;
- I/O and coin/service inputs;
- sound-board communication;
- texture/luma/color-translation windows.

Those ranges should be added from measured STF traces rather than copied
blindly from VF2.

## Build and test

The host build does not require game ROMs:

    cmake -S tools/recovery/i960 -B build/recovery-i960 -DCMAKE_BUILD_TYPE=Release
    cmake --build build/recovery-i960 --config Release
    ctest --test-dir build/recovery-i960 -C Release --output-on-failure

The tests currently exercise:

- i960 instruction decode and formatting;
- memory load/store execution through the generic bus;
- loops and branch flow;
- nested architectural call/return frames;
- fail-closed Model 2B mapping.

GitHub Actions runs the same suite on Linux and Windows.

## Bus contract

The CPU receives a stf_i960_bus with:

- program_image/program_size for instruction decoding;
- a read callback;
- a write callback;
- opaque adapter context.

The current executor therefore preserves the broad VF2-proven instruction
coverage while removing the vf2_model2a dependency.

## Recovery rule

A successful CPU instruction does not imply that an STF hardware access is
understood. If a program reaches a device range that has not been measured,
the Model 2B adapter returns STF_ERROR_UNSUPPORTED. The caller can then turn
that address into a probe target instead of silently fabricating hardware
behavior.


## Corridor runners

Two host tools are built:

    stf_i960_corridor
    stf_i960_probe

Use `stf_i960_corridor` for reproducible recovery corridors with optional
work-RAM preload/dump, register seeding, JSONL traces and final CPU state.

Example:

    build/recovery-i960/stf_i960_corridor \
        --rom rom/rom_code1.bin \
        --entry 0x00000000 \
        --steps 10000 \
        --stack 0x005ff800 \
        --trace out/boot.jsonl \
        --state out/boot-state.json

The runner stops at the first unsupported Model 2B access and reports both a
region hint and, when the address exactly matches the linker map, its STF symbol.

Use `stf_i960_probe` for smaller experiments that need repeatable `--write32`
RAM seeds and `--watch ADDRESS:SIZE` state snapshots.

Resolve named entries after building the original program image:

    nm960 -n temp/rom_code1.out > build/rom_code1.nm
    python tools/recovery/resolve_symbol.py build/rom_code1.nm camera_init

Then pass the returned address to `--entry`.
