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

The first adapter implements only evidence already present in STF:

    ROM       0x00000000 ...
    work RAM  0x00500000 ... 0x005FFFFF

The 1 MiB work-RAM range comes directly from src/lib/rom_code1.ld.

Unknown addresses fail with STF_ERROR_UNSUPPORTED unless an explicit device
callback handles them.

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
