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
The crush-part spawn side is now recovered far enough to complete the portable
lifecycle around the remaining Model 2 math-command boundary. efc_crushpts_speed_cont
is split into an exact CPU request builder and a two-output resolver: the CPU
path clamps body height to 70, applies the six radial/vertical profiles, uses
the original 16-entry angle-offset and jitter tables, preserves the cumulative
radial jitter across records, doubles the jitter contribution to Y, and rejects
counts above four. The former Model 2 math boundary is now closed through the already recovered
cpres1 scalar semantics: command 0x24 is sin(angle)*radial and command 0x25 is
cos(angle)*radial. The composed crush-speed resolver reproduces the original
post-command behavior by toggling the sign bit of the 0x24 output for X,
preserving the CPU-computed Y velocity, and using the 0x25 output for Z. The
lower-level resolver that accepts raw command outputs remains available for
differential experiments.

The speed generator no longer embeds copies of flt_34A4C, flt_34A64,
word_34A7C, or word_34A9C. Those six radial profiles, six vertical profiles,
16 angle offsets, and 16 jitter values are represented by a typed ROM view and
fed explicitly to the portable runtime. Sonic Championship carries the same
contiguous 0x90-byte table block at 0x34A78..0x34B08, a +0x2C relocation from
sfight's 0x34A4C..0x34ADC block. validate_schamp_rom.py pins both the full
block SHA-256 and all decoded entries, so the cross-title address is evidence
from the supplied program ROM rather than an inferred relocation.

The outer efc_crush_parts_put_cont wrapper is composed now as well. The
portable transaction builds all speed requests, executes the recovered
0x24/0x25 scalar trig operations, materializes the velocity triples, and feeds
them directly into efc_crush_parts_set. The Sonic Championship validator also
pins the corresponding relocated program corridor at 0x32174..0x32564,
covering the put wrapper, set transaction, and speed generator together.

The active-slot body of efc_parts_cont is now composed in its original order:
epc_oidasi, epc_parts_pos_calc/sub_3464C, then epc_parts_ang_calc. epc_oidasi's
CPU-visible behavior is recovered exactly: either fighter's +0x7D2 bit 0 skips
the adjustment; otherwise the first two outputs from cpres command 0x77 are
scaled by 0.16 and added to X and Z respectively, leaving Y untouched. Command
0x77 itself remains an explicit coprocessor boundary because the routine emits
nine result words and its internal geometric meaning is not yet proven.

The boundary is now anchored to the DSP image itself. Model 2B's boot loader
uploads 0x3A0E 16-bit words (0x741C bytes, 4954 48-bit SHARC packets) starting
at _cpres_data. sfight places the upload at 0xB6318; Sonic Championship
relocates the header by +0x138 to 0xB6444 and the actual upload starts at
0xB6450. The supplied schamp image hashes to
f86acf80cca9a82cbefb6c8b8f38e4e3a5862cb7b409058075823fea8267fe4d.
The two host-side 0x3B807777 call words are pinned at 0x31E28 (epc_oidasi) and
0x8AFA8 (the projectile collision query). The associated mpr-19015.29/.30
copro_data image is also reconstructed and hash-pinned, so future 0x77 work can
follow the actual SHARC program and collision/height-map data rather than infer
a formula from the i960 consumer. tools/recovery/extract_cpres_program.py emits
either the raw upload or one 48-bit SHARC packet per line for that analysis.

The SHARC dispatch table is now decoded directly from the upload. Command 0x77
dispatches to PM 0x20B1F. Its handler consumes four FIFO floats in order
(X, Y, Z, radius), initializes per-query scratch state, scans the collision
tables rooted at 0x30600 and 0x30700, and returns nine words from scratch slots
+0x1C, +0x1E, +0x1A, +0x19, +0x18, +0x13, +0x15, +0x14, and +0x16. The first
two are the horizontal push-out values consumed by epc_oidasi; the projectile
path preserves the remaining seven as collision metadata. The runtime boundary
therefore models the complete nine-word packet rather than only two synthetic
outputs. The exact semantic names of words 2..8 and the internal scan formula
remain the active recovery frontier. tools/recovery/analyze_cpres_77.py pins
the protocol shape, while disasm_cpres_sharc.py follows the handler itself.

copro_down2 is a separate Geometry SHARC loader (GEO_CTL1/GEO_PROGRAM_START);
it does not replace the cpres program loaded by copro_down.

efc_crush_parts_set is recovered as a composed transaction over the single
0x48-byte +0x88 slot. It reproduces the pre-spawn +0x1F40 part mark, the
per-record delete_parts_weight call before slot/gate rejection, the bit-3/bit-1
spawn gate, position lookup from defender +0x1F4 + part_index*0x0C, ROM-record
radius/object/angle/bounce/floor fields, low-nibble spin selection, and the
post-loop persistence bookkeeping through +0x40[part_index] and the
+0x1F60/+0x1F62/+0x1F64/+0x1F66 history lanes. The word_CE340 spin table is
also exposed through a ROM view at absolute address 0x000CE340.

The first-two-floor-hit side effect is narrowed as well: sub_3FA78 extracts the
high nibble of slot flags, uses scanbit/highest-set-bit selection, and indexes
no_sfx_or_sd_punch_k. The portable runtime exposes that table index as an audio
request instead of invoking the sound backend directly. Recovery tests now
compose speed request/resolution, spawn, position physics, angle update,
visibility projection, draw extraction, and floor-sound selection in one
lifecycle path.

sub_3464C is now recovered across its command-0x29 transform as well as the
post-transform visibility arithmetic. The portable cpres helper models the
observed Model 2 3x4 affine point transform, then the visibility layer projects
X/Y and the part radius by focus_distance / camera_z, rejects negative-Z
points, and recreates the four 0x50A368 edge-visibility bits against the
original +/-248 horizontal and +/-192 vertical limits. The low-level
camera-space entry remains available for differential work, but normal portable
execution can now start from the part's world-space XYZ plus the current camera
matrix. epc_parts_pos_calc's dormant bit-7/bit-3 branch is composed with that
transform too: the wrapper reads XYZ/radius straight from the 0x48-byte slot,
derives the visibility mask, and performs the original deactivate-on-zero cull
without a caller-supplied mask.

epc_parts_pos_calc is now recovered for its CPU-visible physics path as well.
The normal branch subtracts the global gravity term from Y velocity, damps X/Z
velocity by 0.98, then integrates position. Floor contact uses the slot's +0x40
reference (plus the stage floor offset when flag bit 19 is set), increments the
+0x3C contact counter, emits the original sub_3FA78 call site as a portable
event for the first two contacts, reflects Y velocity using
0.2 + 0.2 / bounce_parameter, and damps X/Z by 0.9 while the rebound remains
above the original small stop threshold. Once below that threshold, bit 7 is
set and XYZ velocity is zeroed. Wall handling clamps against stage_x minus the
slot radius, reflects the matching horizontal velocity by -0.3, respects the
original finish_wall_flag bits, and sets bit 19 when the fragment escapes the
outer stage_x + 0.5 bound. The dormant bit-7/bit-3 path can also deactivate the
part when the recovered sub_3464C visibility mask is zero.

The crush/loose-part slot rooted at mod_fa_effect+0x88 is now recovered
across spawn, CPU physics, visibility, angle state, and draw extraction. The structure is one 0x48-byte slot:
+0x00..+0x08 position, +0x0C..+0x14 velocity, +0x1C/+0x1E/+0x20 current
angles, +0x22 object id, +0x24 flags, +0x28/+0x2A/+0x2C target angles,
+0x2E free-spin increment, +0x34 bounce parameter, +0x38 age, +0x3C
ground-contact count, +0x40 floor reference, and +0x44 auxiliary state.
epc_parts_ang_calc is recovered, including its unusual behavior where a large
Y-angle correction divides the step by (ground_contacts+1) and the reduced
step remains live for the following Z correction. efc_disp extraction is also
recovered: flag bit 0 selects the owner fighter and bit 19 requests the
graphics-state save/restore wrapper around set_obj.

The six-slot spark/impact pool at mod_fa_effect+0x790 is also recovered.
sub_327E8 initializes and advances the pool: +0x18 is a countdown and +0x1A
is a frame-table index, both updated once per tick for every active slot.
efc_disp renders active entries from +0x00/+0x04/+0x08 position, resolves the
u16 frame ID through the table token stored at +0x1C, and uses independent
scale words at +0x0C/+0x10/+0x20. A notable original-layout quirk is preserved:
the stride is 0x20 even though +0x20 is accessed, so each slot's Z-scale aliases
the next slot's X-position and the last slot reaches four bytes beyond the
nominal six-slot stride region.

The matching efc_disp consumer is recovered as a portable draw-extraction
stage. It scans the same first 16 particle slots used by sub_32B10. An active
slot emits one draw using +0x00/+0x04/+0x08 as the position triplet, +0x20 as
uniform XYZ scale, and +0x1A as the object/frame id passed to set_obj. Flag bit
2 suppresses the orientation words otherwise sourced from g13, while flag bit
0 wraps the draw in the original graphics-state save/restore sequence. The
portable recovery exposes those two behaviors as draw metadata rather than
writing Model 2 command RAM directly.

The collision-particle backend reached by damage_unit is now recovered through
the CPU-visible allocator/update cycle too. sub_32A5C allocates the first free
0x24-byte slot from mod_fa_effect+0x310, scanning 16 slots normally and 32 only
for also_sub_mode 0x1A/0x1B, while sub_32B10 advances exactly the first 16 slots
each frame. Active slots use +0x18 as age, +0x19 as flags, +0x1A as the current
frame ID, +0x1C as the effect descriptor address, and +0x20 as the current
scale/value. Descriptor layout is float initial value, u16 duration, u16 frame
divisor, then u16 frame IDs. Bit 3 selects the original eight-entry per-age
scale sequence. Expired slots clear both position triplets and all state from
+0x18 through +0x20.

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
