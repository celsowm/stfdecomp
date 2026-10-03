# Model 2B recovery runtime

## Goal

Run increasingly large Sonic the Fighters i960 corridors under a host-side
executor while preserving a hard boundary between recovered CPU semantics,
unambiguous storage, and unknown Model 2B devices.

This is a recovery runtime, not a general Model 2 emulator and not the Saturn
port runtime.

## Components

### Neutral i960 core

Location:

    tools/recovery/i960/

The decoder/executor lineage comes from celsowm/vf2-decomp. The reusable CPU
semantics retain the BSD-3-Clause attribution while the executor no longer
depends on vf2_model2a.

### Generic bus

All CPU memory operations cross:

    stf_i960_bus_read
    stf_i960_bus_write
    stf_i960_bus_read_u32
    stf_i960_bus_write_u32

The bus is the instrumentation and differential boundary.

### Model 2B adapter

The adapter defaults to three classes:

    read-only data
        program ROM
        rom_data.bin
        rom_ep.bin and its corroborated mirror

    raw storage
        work RAM
        buffer RAM
        CPU-control storage
        palette/color-translation RAM
        backup RAM
        texture RAM
        luma RAM

    device / fail-closed
        GEO / geometry program
        copro function/FIFO/SHARC IOP
        copro/GEO/video control registers
        IRQ/timers
        tile/video behavior
        I/O and serial
        render-mode behavior
        sound communication

An address can be known without its behavior being modeled. That distinction is
intentional.

## Why not copy vf2_model2a

VF2 is Model 2A and STF is Model 2B. Recovered i960 semantics and recovery
methods transfer well; hardware assumptions do not automatically transfer.

The contract is:

    recovered i960 semantics      reusable
    memory-access API             reusable
    trace/differential method     reusable
    cross-title constants         evidence
    MAME behavior                 corroborating evidence
    STF Model 2B behavior         must be established for STF

## First corridor workflow

Build and test:

    make recovery-test

Build the original STF image and symbol listing:

    make symbols

The standalone extraction path also works:

    python tools/data_extract.py --rom

Run by symbol through the Python launcher:

    python tools/recovery/run_corridor.py \
        --symbols build/rom_code1.nm \
        --entry camera_init \
        --stack 0x005ff800 \
        --steps 50000 \
        --trace out/camera-init.jsonl \
        --state out/camera-init-state.json

When present, rom/rom_data.bin and rom/rom_ep.bin are attached automatically.

For reproducible experiments use a JSON scenario:

    python tools/recovery/run_scenario.py tools/recovery/scenarios/my-run.json

Task scenarios can name an STF task such as fa_camera or fa_coli. The runner
mechanically resolves its init entry from the assembly task table; it does not
invent task-instance state.

## Fail-closed recovery loop

A normal run stops on the first device access whose behavior is not modeled:

    run
      -> first unsupported access
      -> region/symbol hint
      -> trace/reference experiment
      -> smallest evidence-backed implementation
      -> rerun
      -> differential comparison

Exploratory probe options can accept a measured write or replay a measured
read, but any run that consumes them is marked exploratory.

## Validation

The host build has ROM-independent tests for:

- i960 decode/format/execute;
- load/store and branch flow;
- nested call/return frames;
- bus trace step synchronization;
- fail-closed GEO and control-device accesses;
- buffer/work/CPU-control storage;
- attached main-data/EP ROMs;
- palette/backup/texture storage.

GitHub Actions builds and tests the recovery core on Ubuntu and Windows.

## Relationship to the Saturn port

The recovery runtime answers **what Sonic the Fighters does**.

LibSaturn answers **how to express those semantics on Saturn**.

Model 2-specific addresses, FIFO protocols and device abstractions should not
leak into portable gameplay code. Recovered behavior should cross that boundary
as state machines, fighter/collision/animation semantics and normalized assets.
