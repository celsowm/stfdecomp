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
- common guard path through 0x2AE28, including typed skill and cane-2d sound events;
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
- centralized attack_hit side-exit classification around 0x2AC74/0x2AE40;
- loc_2B8C8 lifecycle rollback of +0x1234 and enemy +0x108 markers;
- composed accepted-hit and rejected-hit transaction tests across strength, combo, damage, sound, reaction, motion and energy application;
- hit-sound selection plan at 0x2B33C..0x2B3FC without embedding the audio backend;
- total_skill_adder_g7/g8 rank/select gating and total-skill accumulation;
- set_kamae_ram wrapper planning: exact get_kamae_value selector/destination requests;
- get_kamae_value motion-record decode, count-stream traversal, opcode execution, and final i960 cvtri conversion;
- ring_tobitiri_set CPU policy: exclusions, 2/4/8/16-ring damage tiers, special-motion profile, sound/drop policy, and 24-slot allocation mask.

These helpers keep genuinely external resources explicit rather than silently
emulating them. Motion-record lookup remains caller-supplied through the
offset_list_motions resolver, and sound playback remains an event for the target
audio backend. The recovered core now includes the stance execution itself,
ring lifecycle/render selection, and sfight/schamp sound-table address metadata.

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
recovered explicitly. The accepted integration transaction now crosses strength
preparation/scaling, combo/skill bookkeeping, contextual damage, sound planning,
finish/reaction classification, normal reaction, knockback motion and final
energy application; the rejected transaction verifies rollback. Guard-common and
combo bookkeeping now share that classifier instead of duplicating its branch
logic. The hit-sound branch now has a typed plan describing which original sound table
and entry/tier is selected. The portable layer also exposes the exact sfight and
Sonic Championship base address for each table family, including the byte-offset
semantics of the list-pointer tables. Table bytes and actual sound playback
remain outside the portable layer. Skill accounting, the set_kamae_ram wrapper, and get_kamae_value are now recovered. The
stance path keeps offset_list_motions resolution explicit, then reproduces the
20x3 descriptor decode, count-stream skips, row-copy/zero semantics, and the
final cvtri pass against caller-provided stance RAM. The CPU-visible half of
ring_tobitiri_set is now recovered as a typed planner, including the 24-slot
spanbit/setbit allocation policy. The spawn half is also recovered: horizontal
atan/Y rotation, the eight local vector records, exact profile
threshold/scale/trajectory selection, and linked-list slot acquire/recycle are
covered by host tests. The ROM-backed A/B/C vertical trajectory tables are now
described without embedding proprietary data: sfight addresses are pinned,
Sonic Championship addresses are corroborated at +0x138, and the exact
sentinel-delimited sample counts are A=99, B=119, C=139. The CPU-visible
per-frame ring_tobitiri lifecycle is also recovered: pause freezes age/physics,
expiry unlinks the slot before simulation, X/Z integrate and bounce at +/-7.5,
zero-height samples damp horizontal velocity by 0.7, the -1.0f trajectory
sentinel transitions the ring to landed state, and the former
visible_from_frame field is corrected to blink_from_frame with the original
two-on/two-off bit-1 cadence. Sonic Championship pins the full 0x78E84..0x791F4
runtime corridor plus the 0x791F8..0x79270 pool bit helpers. The renderer/drop asset-selection layer is now recovered as typed output as
well: normal ring animation IDs, stage-2 secondary IDs, concrete Egg variants
1..4, fixed special variant 5, and the original 16-step spin phase. The
damage_calculation ring event is also composed through a Model 2 adapter into
ring_tobitiri_set, reading the original fighter fields and creating real pool
slots. The accepted attack_hit integration test now continues through damage
application into ring scatter spawn instead of stopping at a boolean request.
The remaining attack-hit frontier is therefore no longer the ring handoff.
The stance wrapper and decoder are composed too: set_kamae_ram planning feeds
get_kamae_value through an explicit offset_list_motions resolver, covering both
the normal four-request path and the bit-29 short path while keeping ROM asset
lookup outside the portable core. That composition is now consumed at the real
attack_hit gate as well: the recovered side-exit classifier clears defender bit
29, emits the stance event, and the integration flow executes the defender
set_kamae refresh before continuing. The previously skipped
0x2B0D0..0x2B124 state prelude is also recovered, including +0x194 and the
conditional +0x122x/+0x124x state propagation before damage transformation.
The contextual-damage block immediately following that prelude is already
recovered. Hit-sound table addressing is pinned too, including the +0x138
Sonic Championship relocation for all five table families, and caller-supplied
table bytes can now be resolved into concrete sound IDs (single entry or
zero-terminated list) without embedding ROM data. The normal reaction path no
longer needs an externally invented sub_2B94C result either: the selector and
its sub_2BA44 down-height override are recovered against caller-supplied
uk_hit_motions tables and composed directly into loc_2B488. The per-hit-kind r7 record is decoded directly from the original
0x50A800 + hit_kind*40 layout as well. One decoded 40-byte profile supplies the
strength scale (+0x08), motion-prefix fallback scales/angles (+0x10..+0x26),
and final horizontal/vertical knockback scales (+0x00/+0x04), so the accepted
integration flow no longer injects manual 1.0 profile constants. calc_mht_adr is
now composed into that same flow: the motion selected by sub_2B94C is searched
for tag 0x11, a found MHT record drives the prefix path, and NOT_FOUND falls
back automatically to the decoded 0x50A800 profile. The original byte_1D006
record-stride table is recovered in the portable core too. The animation_related frontier is now reduced further by a ROM-backed adapter:
the portable runtime can read the original absolute animation pointers from a
caller-supplied image/base-address view, translate them to blob offsets, and run
calc_mht_adr directly. The accepted hit integration path now uses that absolute
pointer representation rather than pre-converted animation_offsets. Sound-table
resolution has the same treatment: a caller-supplied addressable image plus the
recovered sfight/schamp table bases is enough to resolve the concrete sound IDs
without slicing a table first.
 The stance path now has the same
addressable-view treatment for offset_list_motions, and the hit-motion selector
can resolve the original defender-character chain
ptr_DA0B4[character] -> uk_hit_motions[selector] -> 44-word motion table
without a synthetic callback table. The accepted transaction uses those
absolute-pointer views directly. Remaining work in the accepted-hit corridor is
therefore mostly true backend integration: supplying the original data images
and executing target effects such as playback/rendering around otherwise
recovered gameplay state transitions.


The branch transaction suite now resolves guard/down sub_2B94C calls internally:
group 0 for guard-common, group 1 for guard block B, group 4 for generic down,
and group 5 for SPECIAL_BIT16. Generic down also continues through calc_mht_adr
and the shared knockback vector, while guard-common composes total_skill_adder_g7.
