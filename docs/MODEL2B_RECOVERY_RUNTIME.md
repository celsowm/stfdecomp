# Model 2B recovery runtime

## Goal

Run increasingly large Sonic the Fighters i960 corridors under a host-side
executor while preserving an explicit boundary between recovered CPU semantics
and still-unknown Model 2B hardware behavior.

This is a recovery runtime, not an emulator and not the Saturn port runtime.

## Components

### Neutral i960 core

Location:

    tools/recovery/i960/

The decoder/executor lineage comes from celsowm/vf2-decomp, where the same CPU
semantics were exercised against VF2 reference execution. The port retains the
BSD-3-Clause license and renames/removes VF2-specific API surface.

### Generic bus

The executor no longer accepts vf2_model2a. All memory operations pass through:

    stf_i960_bus_read
    stf_i960_bus_write
    stf_i960_bus_read_u32
    stf_i960_bus_write_u32

This is the seam for differential instrumentation and Model 2B device models.

### Initial Model 2B adapter

The adapter implements only evidence already present in STF:

    ROM             0x00000000 ...
    work RAM        0x00500000 ... 0x005FFFFF
    geometry RAM    0x00800000 ... 0x00803FFF
    buffer RAM      0x00900000 ... 0x0091FFFF

The work-RAM range and the geometry/buffer boundaries are derived from
`src/lib/rom_code1.ld`. Geometry RAM and buffer RAM are modeled only as raw
byte storage; no TGP command or rendering semantics are inferred from that.

Declared but still fail-closed device addresses include:

    GEO_PROGRAM_START       0x00804000
    COPRO_SHARC_IOP_START   0x008C0000
    COPRO_CONTROL1_START    0x00980000
    GEO_CTL1_START          0x00980008
    COPRO_STATUS_START      0x00980014
    MIDI_START              0x009C0000
    CPU_CONTROL_START       0x00E00000
    IRQ_REQUEST_START       0x00E80000
    IRQ_ENABLE_START        0x00E80004
    TIMERS_START            0x00F00000

Unknown addresses fail with `STF_ERROR_UNSUPPORTED` unless an explicit device
callback handles them. Faults are tagged with a region/symbol hint from the
linker map.

## Why not copy vf2_model2a

VF2 is Model 2A and STF is Model 2B. The recovered VF2 CPU semantics are highly
valuable, but copying the full Model 2A hardware map would turn useful
cross-title evidence into hidden assumptions.

Instead:

    recovered i960 semantics      reusable
    memory access API             reusable
    trace/differential method     reusable
    Model 2A device behavior      evidence only
    Model 2B device behavior      must be measured for STF

## Next hardware milestones

Add one bounded device range at a time, in this order:

1. geometry/TGP command path around the already shared 0x005010xx runtime
   globals and observed FIFO/port accesses;
2. interrupt/timer/control registers needed to advance deterministic frame
   corridors;
3. input state needed for controlled fighter scenarios;
4. sound command communication only after the executable/traffic is measured.

Each range should ship with:

- a minimal address map;
- trace evidence;
- a fail-closed default for unknown registers;
- a synthetic host test;
- at least one STF reference corridor or snapshot comparison when available.

## Relationship to the Saturn port

The recovery runtime exists to answer what STF means. LibSaturn answers how to
express that meaning on Saturn.

Do not leak Model 2 hardware abstractions into gameplay code intended for the
port. Recovered semantics should eventually cross the boundary as portable
state machines, animation/combat data and normalized assets.


## First corridor workflow

Build the recovery tools:

    cmake -S tools/recovery/i960 -B build/recovery-i960 -DCMAKE_BUILD_TYPE=Release
    cmake --build build/recovery-i960 --config Release

Generate a symbol listing from the normal STF build:

    nm960 -n temp/rom_code1.out > build/rom_code1.nm

Resolve a recovered/disassembled function:

    python tools/recovery/resolve_symbol.py build/rom_code1.nm camera_init

Run from that address with a controlled stack and trace:

    build/recovery-i960/stf_i960_corridor \
        --rom rom/rom_code1.bin \
        --entry 0xADDRESS \
        --stack 0x005ff800 \
        --steps 50000 \
        --trace out/camera-init.jsonl \
        --state out/camera-init-state.json

When execution stops on an unmapped device, the reported address becomes the
next bounded hardware-recovery target. Feed the JSONL into:

    python tools/recovery/classify_tgp_trace.py out/camera-init.jsonl
    python tools/recovery/trace_fields.py out/camera-init.jsonl --base NAME=ADDRESS

This keeps the loop evidence-first:

    run -> first unsupported access -> classify -> model one bounded behavior
        -> rerun -> differential check
