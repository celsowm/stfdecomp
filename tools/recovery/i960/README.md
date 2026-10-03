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
- extension callbacks for device-visible ranges;
- fail-closed behavior for unmapped reads/writes;
- ROM writes rejected.

Not modeled yet:

- TGP/geometry ports and geometry RAM;
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
