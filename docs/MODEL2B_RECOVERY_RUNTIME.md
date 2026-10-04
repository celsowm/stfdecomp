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


## Recovered portable gameplay semantics

The recovery tree now contains a growing ROM-independent C layer under:

    src/recovered/

The following collision/attack semantics are recovered and covered by host
tests:

- coli_init workspace initialization;
- finish_val_cont progression;
- rob_ball_data_make record construction;
- support_rob_position CPU-side prelude;
- no_coli_unit_set suppression mask;
- coli_attack_chk core and Model 2 state adapter;
- attack_hit input/state prefix;
- attack strength byte -> IEEE-754 conversion;
- scalar coprocessor command 0x0D001A1A as square root;
- post-sqrt attack strength scaling;
- guard/counter branch classification;
- guard block A/B CPU-side state;
- common guard path through 0x2AE28;
- combo bookkeeping through 0x2B018;
- contextual damage transform through 0x2B318;
- energy-difference damage scaling and finish-blow detection;
- post-hit reaction classification;
- normal post-hit motion/stun state through 0x2B554;
- down/special reaction state through 0x2B624;
- calc_mht_adr motion-hit table traversal;
- pre-COP motion scaling and angle selection through the 0x2B738 handoff;
- scaled sine/cosine COP semantics and 3D knockback vector through 0x2B898;
- hit-motion row selection plus SNC_DOWN remap (sub_2B94C/sub_2BA44);
- damage_calculation/ketchup energy application and KO/ring-scatter events;
- attack_hit side-exit classification around 0x2AC74/0x2AE40;
- loc_2B8C8 lifecycle rollback of +0x1234 and enemy +0x108 markers;
- composed accepted-hit and rejected-hit flow tests across the portable layer.

These helpers intentionally report external actions such as sound, skill
accounting, stance/motion lookup, or set_kamae_ram as events or explicit input
dependencies rather than silently emulating unrecovered subsystems.

The reference Japanese sfight ROM is validated locally through:

    tools/recovery/validate_sfight_rom.py

No ROM bytes are committed. The validator pins CRCs and selected collision /
attack_hit corridor signatures so the portable recovery remains anchored to the
actual target program.

### Current attack_hit frontier

The main post-hit path now crosses the previous `calc_mht_adr` / motion-physics
boundary. The portable layer recovers the pre-COP motion parameters, the
0x24/0x25 scaled sine/cosine operations, the resulting XYZ knockback vector,
the hit-motion selector/remap, and the shared `damage_calculation` energy
application contract.

The `loc_2B8C8` rollback and its major incoming side-exit predicates are now
recovered explicitly, and integration tests compose both an accepted hit and
a rejected/rolled-back hit across the portable contracts. The next frontier is
therefore the remaining event/orchestration boundary: sound requests, skill
accounting, set_kamae_ram and ring-scatter execution should be lifted as typed
events/contracts so a complete attack_hit transaction can be replayed without
embedding Model 2 devices in gameplay code.
