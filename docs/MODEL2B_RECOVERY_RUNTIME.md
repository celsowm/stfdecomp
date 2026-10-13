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
- collision (0x294D4..0x29534) simultaneous-hit selection, priority-bit arbitration, strength/frame tie-breaks, and ordered dual-hit dispatch;
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
scaled by 0.16 and added to X and Z respectively, leaving Y untouched. The
command-0x77 collision query is now recovered semantically too, so normal
portable frame execution derives those outputs from typed fighter collision
balls instead of receiving them from an external coprocessor stub.

The boundary is now anchored to the DSP image itself. Model 2B's boot loader
uploads 0x3A0E 16-bit words (0x741C bytes, 4954 48-bit SHARC packets) starting
at _cpres_data. sfight places the upload at 0xB6318; Sonic Championship
relocates the header by +0x138 to 0xB6444 and the actual upload starts at
0xB6450. The supplied schamp image hashes to
f86acf80cca9a82cbefb6c8b8f38e4e3a5862cb7b409058075823fea8267fe4d.
The two host-side 0x3B807777 call words are pinned at 0x31E28 (epc_oidasi) and
0x8AFA8 (the projectile collision query). The associated mpr-19015.29/.30
copro_data image is also reconstructed and hash-pinned, so the recovered 0x77
semantics remain anchored to the actual SHARC program and collision data rather
than inferred solely from the i960 consumer. tools/recovery/extract_cpres_program.py emits
either the raw upload or one 48-bit SHARC packet per line for that analysis.

The SHARC dispatch table is now decoded directly from the upload. Command 0x77
dispatches to PM 0x20B1F. Its handler consumes four FIFO floats in order
(X, Y, Z, radius), initializes per-query scratch state, scans the collision
tables rooted at 0x30600 and 0x30700, and returns nine words from scratch slots
+0x1C, +0x1E, +0x1A, +0x19, +0x18, +0x13, +0x15, +0x14, and +0x16. The first
two are the horizontal push-out values consumed by epc_oidasi; the projectile
path preserves the remaining seven as collision metadata. The runtime boundary
therefore models the complete nine-word packet rather than only two synthetic
outputs.

The collision-query formula is now recovered through the complete observable
0x77 result. Each fighter scan has a 3D broad phase: ball 13 acts as the root
center, and the 32-entry fine scan is skipped when its distance from the query
exceeds 3.0f. The fine phase reads one collision-ball center XYZ and radius per
entry, skips zero-radius entries, computes full 3D center distance, and tests
distance <= query_radius + ball_radius. Every overlap contributes to the
horizontal push accumulator. The firmware's register reuse produces the
non-obvious factor 1 - 2*distance/(query_radius+ball_radius), rather than the
geometrically intuitive 1 - distance/sum. The portable semantic command keeps
that factor while using host IEEE arithmetic instead of claiming bit-identical
SHARC RECIPS/Newton rounding.

The nine return words are now named: push X, push Z, last fighter, last ball,
last unit, P0 ball mask, P0 unit mask, P1 ball mask, and P1 unit mask.
Ball masks are remapped to unit masks through each fighter's 32-entry
ball-to-unit map. stf_copro_command77_semantic_bits() reproduces this complete
packet from typed collision-ball state, while the raw nine-word packet remains
available for differential replay. stf_crush_part_frame_model2() now resolves
0x77 internally when collision state is supplied, so the normal composed crush
frame no longer needs caller-injected push values. tools/recovery/analyze_cpres_77.py
pins the broad/fine protocol and names the complete packet, while
disasm_cpres_sharc.py follows the firmware handler itself.

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

### Collision dispatch priority (0x294D4..0x29534)

The portable `stf_collision_hit_order_resolve` function decodes the final
`collision` arbitration after both `coli_attack_chk` calls. It reads the
latches at task-workspace +0x274/+0x276. A single hit goes directly to its
attacker; when both latch, bit 15 of fighter +0x804 has first priority.
When those bits agree, the unsigned byte at +0x822 selects the larger
attacker value; if those agree, the unsigned halfword at +0x808 selects the
smaller value. Complete ties invoke `attack_hit`/`damage_unit` for fighter 1,
then restore fighter 0 and invoke both again. This order is observable and must
not be collapsed into a single hit.

`stf_collision_hit_order_read_model2` extracts the exact six fighter fields
and two latch words from caller-owned buffers without binding game logic to
Model 2 memory. Tests exercise all decision paths and invalid buffer bounds.
The functions return a decision list; they do **not** claim to have recovered
the remaining `collision` callbacks (e.g. kabe_hit, zibaku_ckeck,
gusya_ckeck), and runtime frame integration/differential ROM replay remain
future work.

### Composed collision hit-check / effect stage (0x2A75C..0x2A7D0)

The original accepted `coli_attack_chk` path copies a 12-byte impact point
from defender +0x1F4 + selected_unit*0x0C into `mod_fa_effect+0x7C`,
`mod_fa_effect+0x8EC` and global `0x50A3FC`, sets the effect countdown
at `+0x78` to 10, and copies a second 12-byte vector from the i960 `r5`
indexed source into `mod_fa_effect+0x8F8`. The new portable
`stf_collision_attack_effect_apply_model2` implements those writes from the
hit resolver's selected unit. The second source is **caller supplied**:
the assembler references `(r5)[r6*1]`, but the correct live pointer at this
call boundary has not been established by a differential trace. The recovered
effect API intentionally refuses to invent it. Misses preserve the effect
and global hit-point buffers.

`stf_collision_frame_check_model2` composes `coli_attack_chk` for fighter 0
then fighter 1 using the same mutable collision workspace, applying each
accepted-hit effect in the original check order, then extracting the
`collision` priority/dual-hit event list. This preserves the lockout
interaction (the first hit sets task +0x2AC to 8), and when both are
allowed, the second hit's effect is the final value in the shared slot.
`stf_collision_frame_dispatch` calls a backend-provided hit/damage
transaction once per event in the original dispatch order, with early-exit
handling. Tests cover simultaneous hits, lockout, one-hit and miss paths,
source offsets and all output buffers. This is a portable collision *stage*,
not a replacement for `coli_cont_cop`, sound/video, kabe_hit or the
entire `collision` function.


### Frame collision RAM reset and attack/damage dispatch

The \`coli_ram_init\` routine at the start of the Model 2B geometry collision
corridor is now a bounded, ROM-independent C adapter:
\`stf_collision_ram_init_apply_model2\`. It reproduces the exact i960
\`movq/stq/stl/stos/st\` zero-write ranges at task +0x170..0x1AF and
+0x98..0xAB, and each fighter +0x75C..0x75F and +0xA68..0xA6F.
It must execute at the original call site **inside** \`coli_cont_cop\`,
not at the end of the frame (when post-damage state is live).

After \`stf_collision_frame_check_model2\` has generated the ordered hit
events, \`stf_collision_combat_apply_model2\` now composes the recovered
\`attack_hit\` prefix (workspace bit-0 gate, +0x1234 counter and enemy
+0x108), an explicit per-pair **attack_hit continuation hook**, then the
recovered \`damage_unit\` accumulation and post-state runtime. It preserves
the original fighter-1/fighter-0 order on simultaneous tie outcomes.
The continuation hook is mandatory: attack_hit's guard/motion/sound paths
cannot be treated as a no-op, and they can change workspace +0x26C before
damage_unit. The sub_19888 special-opponent branch remains exposed in
the prefix result rather than fabricated. Partial failure records whether
the prefix and continuation ran and how many damage transactions completed;
writes before failure are not rolled back.

Tests cover simultaneous mutual impacts into separate fighter damage arrays,
special-opponent/bit-0 skips, per-hit continuation gating, order prevalidation
and partial-completion reporting. No ROM bytes or hidden device assumptions
are embedded.


### calc_attack_flag recovered from coli_cont_cop (0x2DC4x)

The caller-owned, addressable Model 2 view now resolves the original
\`dword_CE020[+0x820]\` 32-bit profile and the 32 per-ball selectors pointed
to by fighter +0x1D18. The reconstructed \`calc_attack_flag\` checks motion
kind bit 8, current frame +0x1AA >= threshold +0x808 and special kind 41,
then writes the source profile into fighter +0xAC8 and the 32-bit selection
mask into +0x768. The i960 bit-test index uses the low five bits of each
selector (0..31). On a disabled profile both fields become zero.

\`stf_collision_copro_attack_prepare_model2\` composes both fighter
\`calc_attack_flag\` calls followed by the previously recovered
\`coli_ram_init\`; it validates both profiles and selector tables before
mutating the frame. This stage must run *after* the original coprocessor's
initial 0x1F003E3E response and *before* area_coli and decide_coli_kind.
Those Model 2 commands are not silently synthesized.

\`stf_collision_frame_check_computed_model2\` now consumes the actual stored
+0xAC8 profiles to run the prior hit checks and arbitration, removing manual
attack_profile_bits injection from that flow. The integration tests run
addressable profile lookup -> two fighter masks -> RAM reset -> two
attack checks -> dual-hit ordering, along with inactive modes and bounds.
The TGP collision outputs, suppressions, r5 impact vector and remaining
attack_hit body still need native execution/differential replay.


### Recovered area_coli geometry query and FIFO response prefix

\`src/recovered/collision_area.[ch]\` recovers the first
\`area_coli\` query/response window from 0x2DCB0 to the next TGP command
at 0x2DEE8. The exact six-word request begins with 0x38007070, followed by
fighter +0x1C, stage_soko, stage_x, adjusted cage height and a per-fighter
phase selector. The cage calculation subtracts the original 0x3E428F5C
float and adds 10.0f when phase <2 and motion bit 9 is clear. Phase 3 sets
the selector's high bit.

The external geometry provider supplies **18 FIFO output words**, which
the recovered CPU side distributes to workspace +0x118..+0x124 and fighter
+0x670/+0x710, +0x660..+0x66F, +0xA58..+0xA67,
+0x650..+0x65F, +0x614/+0x674/+0x624/+0x618. The following
\`ac_make_vect_outset\` path is also recovered: phase-specific tangent
inversion, negative-zero normalization, float sums at +0x644 and +0x64C,
and direction-dependent zeroing of selected +0x650/+0x658 plane words.
The special game-mode predicate is an explicit \`force_flat_mode\` input.
It is not inferred without the caller's live also_mode/sub_mode.

\`stf_collision_copro_area_stage_model2\` connects the previously recovered
two-fighter \`calc_attack_flag\` and \`coli_ram_init\` stage to two sequential
\`area_coli\` packet submissions. Device responses are mandatory and passed
through a callback. If the second response fails, the first fighter's writes
remain, and \`completed_areas=1\` records that boundary. Invalid static
buffers/profile lookups fail before the first mutation. Request/response
ordering is tested with a deterministic fixture, including slope signs,
normal/inverted phases, zero suppression and a failed second callback.

**Boundary:** the original coprocessor's initial 0x1F003E3E and
0x1D003A3A packets still precede this C seam. Hardware responses are
not guessed, and later area_coli 0x15802B2B queries, angular side
selection, decide_coli_kind and position recalculation remain unrecovered
in this runtime.


### area_coli 0x15802B2B scalar query, side-plane selection

At the area_coli boundary immediately after the 18-word response, the
next Model 2 copro query writes 0x15802B2B followed by fighter +0x1F4
(XYZ) and a zero origin (XYZ). One FIFO result is written to fighter
+0x680. This CPU packet and state store are now exposed through
\`stf_collision_area_distance_query_build_model2\` and
\`stf_collision_area_distance_response_apply_model2\`.
\`stf_collision_area_distance_semantic_bits\` supplies a host-only
three-dimensional Euclidean distance for controlled experiments, with
no claim of cycle- or bit-exact Model 2 math.

The original unsigned, sign-stripped branch choice between planes
+0x644 and +0x64C is exposed by
\`stf_collision_area_plane_choose_model2\`. The later angle lookup and
unit-sort remain beyond this contract.

\`stf_collision_copro_area_stage_model2\` now supports an optional
measured \`distance\` callback. When supplied, the original execution
order is area0 -> distance0 -> area1 -> distance1 (not two areas followed
by two distances); scalar writes go to +0x680 and \`completed_distances\`
tracks partial execution separately from \`completed_areas\`. The callback
must represent a real or controlled geometry result. An absent callback
retains the existing prefix-only behavior without guessing hardware.
The integration test covers both ordered paths, 3-4-5 distance,
branch classification, buffer guards and second-query failures.


### area_coli 0x2DF18..0x2E074 signed-angle quadrant lookup and minimum-plane scan

\`src/recovered/collision_area_side.[ch]\` now covers the ROM-grounded
X/Z side path after the area scalar 0x15802B2B query. A correction to the
older \`stf_collision_area_plane_choose_model2\` restores the actual
\`cmpr / bl\` branch direction: among two nonzero absolute plane values,
**the larger one takes precedence**, ties choosing X; X=0/Z!=0 chooses
Z. This behavior is covered by changed differential-oriented host tests.

For an X plane the routine uses the signed +0x26 fighter heading and
sign of (+0x644 XOR heading), extracting an index from bits 12..14 and
sign bit 3; the Z plane uses 0x4000 minus heading, XORed against the
high halfword of +0x64C. The 16-byte \`byte_2E260\` lookup is
**caller supplied** and bounds checked, not embedded in the portable code.
It updates the exact CPU-observable +0xA08 orientation flag,
+0xAE2 low direction, task +0x1F0/+0x1F4/+0x23C halfwords for the fighter,
and the encoded minimum of four candidate +0x650..+0x65C plane floats
at task +0x1F8. The minimum scan uses the actual \`cmpr/bl\` tie behavior:
equal values select the later record. NaN/infinite candidate floats fail
closed without partial writes; the absent-plane path explicitly reports
no orientation instead of inventing a heading.

The existing two-fighter stage now optionally takes the table and runs
area0 -> distance0 -> side0 -> area1 -> distance1 -> side1 in order,
recording completed side stages. Requests lacking the table preserve the
previous area/distance-only API. The later opponent minimum scan at
0x2E074..0x2E0F4 and its \`word_2E230 / word_2E250\` lookups remain
separate pending a matched trace; their output is not fabricated.


### area_coli opposing-plane tail at 0x2E074..0x2E0F4

The recovered area-collision path now covers the final CPU-side opposing
fighter plane scan and 16-way classification. The ROM-independent
\`stf_collision_area_opponent_apply_model2\` reads four opponent planes
(+0x650..+0x65C), selects their minimum with the original
last-record-wins tie rule, and stores the selected IEEE-754 bits at
task +0x204 (fighter 0) or +0x208 (fighter 1). For fighter 1 the
minimum plane index flips bit 0 as in the i960 \`not/and/or\` sequence.

The original task +0x1E0 angular classification is recovered:
\`((angle ^ (angle << 1)) >> 12) & 4\` supplies one direction bit;
\`word_2E250[angle >> 13]\` supplies another; their OR with the
opponent plane index addresses \`word_2E230\`. The two original
ROM tables are explicit caller-provided eight- and sixteen-word views,
with strict index/bounds validation. Task +0x1FC / +0x1FE receives
the composed index; +0x200 / +0x202 receives the lookup result.
No ROM bytes are embedded in the portable library.

The composed path executes **area0 / distance0 / side0 / opponent0**
before starting **area1 / distance1 / side1 / opponent1**. The first
fighter therefore scans opponent planes remaining from the previous
frame, exactly as in the original sequential assembly. The second
fighter scans fighter 0's newly computed planes. The result exposes
\`completed_opponents\` and per-fighter opponent records to diagnose
failed mid-frame iterations. If the table pointers are absent,
the previous area/distance/side prefix behavior is retained.

An earlier recovery error in \`area_coli\`'s own minimum-plane
encoding is also fixed: \`subi r11,0,r10; addi 4,r10,r10\` gives
indices 1/2/3 for the three compared candidates, not -8/-4/0.
The packed task +0x1F8 value now uses the original low index shifted
left four bits. A controlled ROM trace remains necessary to establish
how the inherited r9 register affects lower direction bits in all
call paths; this implementation does not claim that unknown state
to be proven by the new table scan.


### decide_coli_kind profile classification and unit_to_ball expansion

\`src/recovered/collision_kind.[ch]\` recovers the CPU-visible decision
tree at 0x2CFD0..0x2D248. It distinguishes NORMAL, FIX_0/1, RISE_0/1,
RIDE_0/1, BODY/BODY_0, KABE/KABE_0, JUMP_0/1 and TOBIKOE_0/1,
preserves decision bits in the i960 g6 word, and tests the original
fighter flags at +0x1A4, +0x70C, +0x804 and corresponding opponent/
motion checks. BODY/KABE takes the relevant plane index from the
original eight-element directional mapping, reads the appropriate
+0x650 plane and compares it against the original 0.1 threshold.
No player motion or live game state is guessed.

The subsequent original \`unit_to_ball\` helper walks a 32-unit map
with eight-byte descriptors; it maps each encoded byte >> 2 to a
32-bit ball selection bit, with a zero *after the first byte*
terminating a row. Host mapping is supplied explicitly by the caller,
not loaded by guessed ROM addresses. The recovered application stage
expands both primary masks at workspace +0xC0/+0xC4 (inverted), the
auxiliary masks at +0x264/+0x268 and copies associated profile scalars
to +0xCC/+0xD0/+0xC8/+0x88. For the special 0x2D005A5A device
command, a callback is REQUIRED when nonzero source magnitudes would
invoke the device. Unsupported devices fail closed.

The existing two-fighter \`stf_collision_copro_area_stage_model2\` can
optionally perform \`decide_coli_kind\` after BOTH area_coli calls.
This preserves the assembly order:
area0 -> area1 -> decide_coli_kind. The extra step remains opt-in
and exposes \`completed_kind\` and the selected kind; earlier
area-only clients work without additional state or tables.

Remaining limitations: direct differential trace comparison against
STF ROM and hardware-fidelity math are needed; the portable mapping
does not embed original proprietary ROM tables.


### Composed collision-to-combat frame corridor

\`src/recovered/collision_pipeline.[ch]\` now provides a single
buffer-validated frame transaction across the recovered Model 2 stages:

1. \`coli_cont_cop\` / \`calc_attack_flag\` / \`coli_ram_init\` /
   \`area_coli\` / optional \`decide_coli_kind\` (via the existing typed
   geometry providers and externally supplied ROM tables).
2. \`no_coli_unit_set\` for fighter 0 and then fighter 1, populating
   both +0x6F8 suppression masks from fighter/mode inputs.
3. \`coli_attack_chk\` for fighter 0 and fighter 1, including the
   accepted hit effects and original priority/dual-hit arbitration.
   Their attack profile masks come **directly** from
   \`calc_attack_flag\`'s +0xAC8 outputs, not injected a second time.
4. \`attack_hit\` prefix, the mandatory real continuation callback and
   \`damage_unit\` for each ordered accepted hit (fighter 1 precedes
   fighter 0 on a dual-hit tie).

The pipeline insists that the area, frame-check and combat configurations
share the SAME fighter buffers and workspace, with identical size
contracts. It validates essential callback and output sizes before
the first geometry write. Explicit progress reporting distinguishes
the completed area stage, 0/1/2 suppression masks, completed hit
checks and ordered combat attempts/completions. External coprocessor
requests are **never silently approximated**, and attack_hit's missing
body is still a mandatory continuation, not a success stub.

Host regression tests include a no-hit frame, a dual-hit frame
with computed ROM-view attack profiles and deterministic ordered damage,
preflight alias rejection, failure of the first hit continuation,
and failure of the second geometry query. ROM-vs-runtime differential
replay is still needed to substantiate original arcade equivalence.


### attack_hit's sub_19888 special-opponent continuation

\`src/recovered/attack_hit_special.[ch]\` closes the genuine
\`opponent+0x19F==0x12\` early-return branch, instead of accepting a
no-op callback in the middle of attack_hit. The CPU states from
\`sub_19888\` and \`sub_1980C\` are now updated directly: original
tag-5 animation record, signed motion direction bit, attacker +0x198,
+0x1C, +0x822, defender +0x198, state flags +0 and +0x1A4.
When attacker state bit 2 was set, it also computes +0x84 from the
current height and clears the original state/motion bits.

The original \`calc_mht_adr\` lookup is supplied as an explicit callback
that returns the tag-5 record from the caller's actual animation
data. When the moving branch requires the TGP
\`0x2D805B5B\` operation, a separate callback receives the exact
original 3-word input (signed heading, vector +0x80, vector +0x88)
and returns the two float words for +0x18/+0x20 updates.
No hit animation addresses or coprocessor outputs are fabricated;
missing dependencies fail before the special-path state writes.

\`stf_collision_combat_apply_model2\` now optionally routes only
the defender-state-0x12 case through the dedicated special path.
Unlike the generic attack_body callback, that branch returns after
\`sub_19888\` and leaves workspace +0x26C bit 0 clear, so the
following recovered damage_unit correctly skips damage.
All other attack_hit branches preserve the existing mandatory
generic continuation. Tests exercise no-device, mandatory-device,
callback failure and ordered-combat integration.


### Normal attack_hit strength/profile + guard classification prelude

\`src/recovered/attack_hit_normal_entry.[ch]\` composes previously
isolated CPU recovery contracts from the normal \`attack_hit\`
\`0x2A8F8\` to \`0x2A9F0\`: the i960's +0x82A/+0x26 heading
addition, +0x822 strength byte float conversion, workspace +0x26C
bit0 activation, ROM-backed 40-byte hit-kind profile decode at
\`0x50A800\`, and guard classification based on fighter and opponent
live flags. It returns the selected guard route (EXIT, 2AC74,
2AA70, 2AB54) and the semantic host strength multiplier. The
host float calculation is **not** an exact surrogate for the
\`0x0D001A1A\` Model 2 coprocessor sqrt operation; unlike the
real device result, it is explicitly marked approximate.

\`stf_collision_combat_apply_model2\` can now optionally invoke
that prelude for each non-special accepted hit when provided the
original per-kind profile byte table. The genuine sub_19888 special
path keeps priority and bypasses the normal prelude. The existing
\`attack_body\` callback remains **mandatory** for all normal
branches: a guard classifier alone is not a complete reaction,
damage, sound, or motion continuation. The result records
\`normal_entry_applied\` separately, including partial callback
failures. With no table, old consumers work unchanged.


### Recovered BLOCK_A / BLOCK_B live guard branches

The combat runtime can now opt into an original guard continuation
via \`guard_tail\`, in addition to the normal ROM-backed hit profile
and prelude. For the two proven paths selected at
\`0x2AA70\` (BLOCK_A) or \`0x2AB54\` (BLOCK_B), the runtime
executes \`stf_attack_hit_guard_apply_resolved_model2\` directly.
This reuses the existing ROM-recovered +0x26C flag bits (0, 2, 3),
defender +0xC6C/+0xC70/+0x198/+0x6D8/+0x5DE stores, attacker
+0x194 and conditional +0x121C/+0x1220/+0x1244 copies. The
BLOCK_A fixed motion does not require a lookup. For BLOCK_B,
the resolver must supply the actual \`sub_2B94C\` group-1 motion
before the guard-state writes; missing/out-of-range ROM records
fail without writing the guarded state.

After these recovered CPU writes, the **specific**
\`guard_tail\` continuation is invoked, instead of rerunning the
generic \`attack_body\`. The tail still has to implement the vector
coprocessor and audio/effect actions at \`loc_2B738\`; this C
integration does **not** silently treat the branch as completed.
The guard-state result reports the knock sound event but does not
play audio. On failed guard-tail callbacks, partial progress is
explicit (\`guard_applied=true\`, \`guard_tail_completed=false\`);
original already-performed writes are not rolled back. Non-guard
paths and deployments without guard_tail keep the previous
generic callback unchanged. This is a CPU-state recovery milestone,
not full 1:1 arcade execution.


### Guard-vector TGP recovery (loc_2B738 through loc_2B898)

\`src/recovered/attack_hit_guard_vector.[ch]\` now recovers the
post-BLOCK_A/B CPU-visible force-to-velocity sequence at 0x2B738.
The guard branches enter with \`r8 = r6 = r11 = 0\`; this permits
recovery of the five-command device chain independently of the still
missing normal-hit state. The scalar magnitude starts with
\`4 * r9 / (60 + (defender+0x5D8 - 60)*0.3)\`; the preceding
\`attack_hit_normal_entry\` supplies a *host semantic estimate* for
r9, which is not yet the original TGP bit-exact output. The wrapper
sends \`0x0D001A1A\`, \`0x12002424\`, \`0x12802525\`,
\`0x12002424\`, \`0x12802525\` with the recovered sequential
arguments, including the negative combined heading, through a
caller-supplied TGP provider. Three final response words are
committed to defender +0x5E4 (vertical) and +0x5E0/+0x5E8
(horizontal). Failed, nonfinite or incomplete device replies do
not overwrite any of those three values; the number of successfully
completed commands is retained for replay analysis.

When attacker +0x7D2 bit0 is set, the original two-float row
\`flt_2B904[hit_kind]\` is supplied as a ROM byte view: the first
float scales vertical velocity, the second scales both horizontal
components. Tables are checked strictly by byte count, and neither
TGP trigonometric behavior nor proprietary ROM float rows are
guessed.

\`collision_combat_runtime\` accepts optional \`guard_vector_tgp\`
and performs this sequence after the recovered guard-state stores
and before the mandatory \`guard_tail\`. The latter must still
handle remaining damage-calculation/sound side effects, so this
is **not** the end-to-end replacement of attack_hit.
The earlier profile-only and guard-state-only workflows remain
backward compatible.


### Common guard branch 0x2AC74 and abort 0x2B8C8 in ordered combat

\`stf_collision_combat_apply_model2\` can now opt into the common
guard branch (\`STF_ATTACK_GUARD_BRANCH_2AC74\`) using
\`guard_common_tail\`, the existing ROM-backed \`guard_motion_resolver\`,
and the original guard-limit value supplied by its caller. The
already-recovered \`stf_attack_hit_guard_common_apply_resolved_model2\`
handles the CPU-side animation selection, stun duration, counters,
guard motion and flags, returning explicit audio/skill requests.
The continuation callback is still **mandatory** for live TGP,
audio, scoring and other unported effects.

The initial \`+0x860 bit19 && +0x822 == 0\` check is made before
the ROM motion lookup. An abort follows original \`loc_2B8C8\`
through \`stf_attack_hit_abort_cleanup_apply_model2\`: undo the
prefix +0x1234 counter increment and clear BOTH enemy +0x108
activity bytes, without invoking the motion resolver or a
guard/audio continuation. The caller then reaches the usual
post-attack \`damage_unit\` stage, which retains its own workspace
gate. The composed result explicitly distinguishes a completed
common-guard state, early aborted event and completed tail.
Other guard and non-guard continuations remain unchanged.


### Normal 0x2AE40 side exits integrated into ordered combat

`collision_combat_runtime` now has an **opt-in**
`normal_exit_tail` continuation for `STF_ATTACK_GUARD_EXIT`.
After the ROM-backed profile/strength prelude, it executes the already
recovered `STF_ATTACK_SIDE_EXIT_NORMAL_2AE40` predicates before the
tail: early `loc_2B8C8` abort restores the prefix lifecycle counter
and clears both enemy +0x108 bytes without calling any tail. A continuing
path exposes the defender bit29 clear and `request_set_kamae`
event to the mandatory supplied continuation. Other branch routes
retain their existing callbacks; without opt-in, the legacy
`attack_body` path is unchanged. This is **not** full combat recovery:
the callback still owns stance refresh and other remaining operations.
Integration tests cover ordered execution, abort, bit29 consumption,
early guard, failure state, preflight and backwards compatibility.

The resolved 0x2AC74 common-guard helper now checks the abort gate
**before** attempting `sub_2B94C` motion lookup, including when invoked
outside `collision_combat_runtime`. A skipped transaction never queries
a ROM resolver or adds skill. Invalid buffers are rejected even on the
skip route.


### Secondary Sonic Championship guard/exit ROM anchors

`tools/recovery/validate_schamp_rom.py` now checks three independently
hashed i960 intervals recovered from the local `schamp` EPROM pair:
`0x2AC74..0x2AE40` (common guard),
`0x2AE40..0x2AF24` (normal side exit), and
`0x2B8C8..0x2B8F8` (abort cleanup).
These ranges are **Sonic Championship** evidence only; matching SHA-256
digests do not establish that `sfight` has identical instruction bytes or
that the still-callback-backed full attack body is 1:1.


### Recovered stance refresh and guard skill in ordered combat

`stf_collision_combat_apply_model2` now offers two separately opt-in
continuations that replace more of the original event-tail responsibilities:

- `recover_normal_stance` consumes `request_set_kamae` immediately after
  the 0x2AE40 classifier, before the required normal tail. The recovered
  `stf_attack_hit_apply_stance_event` composes `set_kamae_ram` and
  `get_kamae_value` for the current defender using caller-owned
  `normal_stance[slot]` selectors, stance RAM and ROM resolver. Both slots
  are prevalidated, including full selector and output capacity, before
  combat side effects. Skipped and abort paths never resolve ROM motions.
  Resolution failures preserve partial prefix/bit29 state and do not
  proceed into the tail or `damage_unit`.
- `recover_guard_skill` applies existing recovered
  `stf_total_skill_add` rank/select gating on completed common-guard
  paths, updating the caller-owned `total_skill` before the remaining
  mandatory guard tail. Aborts do not add skill; failed tails do not roll
  back previously committed accounting.

The flags are **opt-in** to preserve existing caller semantics. When
enabled, corresponding callbacks must not repeat the stance or skill
operation. Both still require the caller-supplied ROM data and the
unrecovered gameplay/audio/TGP continuation; neither constitutes 1:1
restoration of `attack_hit`.


### Integrated normal hit CPU followthrough (after 0x2AE40)

The opt-in `recover_normal_hit` switch in
`stf_collision_combat_apply_model2` now executes the recovered
`stf_attack_hit_normal_followthrough_apply_model2` contract before the
mandatory `normal_exit_tail`. Its order is:

1. Consume the already-classified normal side exit once and perform the
   original combo/workspace mutations without re-evaluating the bit29 gate.
2. Apply 0x2B0D0 state prelude, including +0x194 and 0x122x/0x124x copies.
3. Transform caller-supplied raw damage according to recovered
   0x2B0F8 context (combo/down, counter, air, ukemi, etc).
4. Run the recovered finish-blow test, including attacker +0x7D2 bit0.
5. Only for non-finish hits, classify reaction branch; for the
   normal reaction, resolve original hit-motion data via a mandatory
   caller-supplied ROM motion resolver and write defender +0x198/+0x5DE.

Other reaction branches (generic/down/special), finish-specific effects,
actual damage_calculation, ring-scatter, TGP knockback, sound and animation
backend events remain the responsibility of `normal_exit_tail` and the
rest of the host runtime. The callback **must not repeat** completed CPU
operations when `recover_normal_hit` is enabled. It still must execute
the remaining original continuation before reporting success.

The caller must provide `normal_initial_damage[attacker_index]`,
`normal_energy_max`, `normal_disable_energy_scaling`, and
`normal_hit_motion_resolver`; a missing resolver or incompatible
configuration fails preflight without side effects. On a later motion
resolution failure, `normal_hit` records which earlier CPU stages
completed; state is intentionally not rolled back, and neither the
remainder callback nor `damage_unit` is executed. The legacy path
remains unchanged when opt-in is off.

The integrated combat regression now covers the ordinary hit, counter
mode 3, KO bypass, generic branch, resolver failure, early abort,
missing resolver, failing continuation, and opt-out compatibility.
This is semantic CPU recovery, **not** proof of binary-matching or
complete 1:1 Model 2 gameplay.


### Non-guard damage, KO and ring-scatter in ordered combat

After the optional normal-hit CPU followthrough, the combat adapter can
also execute the original energy-application stage. Set
`recover_normal_damage = true`, supply `normal_game_timer`, and provide
a **real** `normal_damage_event` callback. This mode requires
`recover_normal_hit = true` and its existing ROM-backed motion resolver.

The `normal_exit_tail` becomes the **pre-damage** continuation: it must
finish every original sound, SHARC/TGP/vector or other operation that
occurs **before** `damage_calculation`, but MUST NOT itself subtract
energy or repeat the damage stage. After that tail succeeds:

1. `stf_damage_calculation_apply_model2` records the scaled damage in
   attacker +0x1F70, conditionally subtracts defender +0x1AC, updates
   defender +0xC54, clamps a knockout to zero and sets fighter bit5.
   The caller-supplied game timer and character-16 exception are observed.
2. When `recover_normal_ring_scatter = true`, the original ring
   request is immediately passed to the existing recovered
   `stf_ring_scatter_apply_damage_event_model2`. The caller supplies
   a live 24-slot `normal_ring_pool`/`normal_ring_slots` pair and
   the stage plus 16 deterministic random words per **defender** slot.
   The stage handles character-dependent suppression, ring counts,
   trajectories and recycled slots; animation/sound output remains external.
3. The mandatory `normal_damage_event` receives the damage and ring
   results before `damage_unit`. When ring recovery is disabled, this
   callback must itself handle `request_ring_scatter` through the real
   engine. When enabled, it still must service `request_ring_sound`
   and any other post-damage external events. Returning false terminates
   the hit with earlier mutations intentionally visible.

`recover_normal_ring_scatter` requires `recover_normal_damage` and
live pool/slot pointers. Missing callbacks or other required dependencies
are rejected before prefix mutation. Normal early-abort and special-hit
paths do not invoke these stages. With both flags disabled, the previous
callback-owned behavior is untouched.

In addition, the recovered CPU followthrough now implements generic
down, down-combo, and special-bit16 reaction branches, using the original
hit-motion group 4/5 resolver and the down-combo route predicate. The
original SHARC/device work remains in the pre-damage tail by default.
The opt-in generic/down `calc_mht_adr` and host-derived impact-vector
continuation described below can now replace that specific portion.
This semantic CPU recovery does **not** make the full game path 1:1. The tests cover group selection, fixed-motion
overrides, damage-reduced stun, early abort and resolver errors, plus
active/inactive timer energy, KO, character 16, ring spawning and
suppression, and event-failure boundaries.


### ROM-backed `calc_mht_adr` and knockback continuation (opt-in)

Set `recover_normal_knockback = true` to integrate the already
recovered `stf_attack_hit_motion_prefix_resolve_rom` and
`stf_attack_hit_motion_vector_compute` helpers into the normal
attack-hit ordered executor. Requires `recover_normal_hit = true`,
the normal profile table, the real remainder callback, and a valid
`normal_knockback_rom` view.

**Host-supplied runtime context:** Set
`normal_knockback_context[attacker_index]` using the actual original
horizontal/strength magnitude (`initial_r9_bits`), combo clamps
(`combo_start`, `combo_sub`, `combo_limit`,
`limit_xang`) and `normal_knockback_heading[attacker_index]` for
the TGP-derived heading. These inputs are *not* inferred from placeholder
constants. The runtime reads updated hit mode, defender flags,
down-combo count and defender +0x5D8 from the actual fighter records;
the impact vector's attacker state and horizontal/vertical scale
come from the recovered fighter and the decoded hit-kind profile.

**Dispatch:** Only generic/down reactions that already successfully
applied their CPU state and returned `requires_calc_mht` enter the
additional motion corridor. It takes the selected motion **from the
completed reaction**, avoiding duplicate group-4 ROM lookup or double
increment of defender +0x6F5. The ROM view follows the original
`animation_related[selector & 0x1fff]` absolute pointers and
searches the motion stream for tag `0x11` with recovered record
strides. When the tag is not present, the original hit-kind fallback
profile is used; malformed tables are an error rather than a fallback.

When the input and ROM computation succeed, all three horizontal
and vertical impact outputs are written to defender +0x5E0,
+0x5E4 and +0x5E8 together, before `normal_exit_tail`.
The result preserves stage flags
`normal_knockback_started`,
`normal_knockback_motion_resolved` and
`normal_knockback_vector_applied` for differential replay.

A failed ROM lookup or non-finite computation never overwrites any
of the three defender vectors and suppresses the remainder callback
and downstream `damage_unit`; earlier combo/reaction mutations are
not rolled back. Invalid opt-in configuration (including missing
ROM view and disabled `recover_normal_hit`) fails before the first
prefix mutation. Normal hits, special-bit16 paths, KO/finish
branches and early side exits do not use this generic/down corridor.
With the flag off, the legacy callback still handles this vector.

**1:1 caveat:** The portable `copro_scalar` trigonometry and square
root approximate the observable CPU/TGP behavior. They are not
proof of bit-exact command ordering, hardware rounding or timing.
The remaining callback must perform authentic sound, gameplay events
and any other undisplaced device work; when this mode is on it must
not re-execute the completed motion/vector writes.

The combat regression includes ROM tag found, tag not found (fallback),
malformed ROM, branch non-entry (normal/special), preflight errors,
legacy opt-out and early side-exit abort.


### Exact reply injection for the five Model 2 TGP knockback commands

The generic/down-hit knockback corridor can now operate in two modes:

- The existing default path calls the portable `copro_scalar` sqrt/sin/cos
  approximations. This is useful for cross-platform semantic testing and
  **does not** establish hardware bit-exactness.
- When `recover_normal_knockback = true` and the optional
  `normal_knockback_tgp` callback is set, both the scalar-sqrt stage
  within the ROM-backed `calc_mht_adr` motion prefix and all four
  subsequent scaled sin/cos requests use replies **supplied by the caller**.
  The provider is responsible for the real Model 2 TGP transport, or an
  authentic pre-recorded trace. The portable runtime does not silently
  substitute host trigonometry on callback failure.

The recovered command sequence is:

| Index | Opcode | Argument words | One reply |
|---|---|---|---|
| 1 | `0x0D001A1A` | computed sqrt operand | magnitude |
| 2 | `0x12002424` | reaction vertical angle, magnitude | Y sin |
| 3 | `0x12802525` | reaction vertical angle, magnitude | horizontal cos |
| 4 | `0x12002424` | -horizontal heading, horizontal cos reply | X sin |
| 5 | `0x12802525` | -horizontal heading, horizontal cos reply | Z cos |

The callback contract remains the established
`stf_attack_hit_guard_vector_tgp_fn` query/reply interface.
`normal_knockback_tgp_completed` reports successful command replies
(0..5); a rejection or non-finite reply stops the continuation.
No partial X/Y/Z vector is committed. The earlier attack-hit and
reaction writes are intentionally **not rolled back**. The final three
vector outputs still pass through recovered CPU multipliers and state
flags. The pre-damage callback, damage calculation and damage_unit are
not reached on an unsuccessful query.

The existing motion prefix and vector APIs remain backward compatible:
`stf_attack_hit_motion_prefix_resolve_rom_with_sqrt` and
`stf_attack_hit_motion_vector_compute_with_tgp` add dependency
injection without changing the old portable entry points. Regressions
use an explicitly **synthetic** five-response transcript to assert
opcode order, argument forwarding, atomic vector writes, error
boundaries and lack of TGP calls on irrelevant branches. Synthetic
responses are not ROM/hardware reference values. Establishing actual
bit-for-bit equivalence requires capturing the same command/reply
sequence on the original Model 2 hardware or a validated SHARC
emulator and comparing each 32-bit reply and end-of-hit state.


### Strict TGP command capture, replay and differential comparison

The recovered C library now exposes `tgp_command_trace.h`, a bounded
adapter for the existing `stf_attack_hit_guard_vector_tgp_fn` callback.
It works with both `guard_vector_tgp` (five-command guard vector) and
`normal_knockback_tgp` (one sqrt plus four trigonometry commands).
It does **not** synthesize hardware results.

- **Capture:** initialize with
  `stf_tgp_command_trace_begin_capture(trace, buffer, capacity,
  real_tgp_callback, userdata)`. Point the combat model's TGP callback
  to `stf_tgp_command_trace_callback` and its user data at `trace`.
  Successful hardware queries append `{opcode, argument_count,
  raw_words[2], reply_bits}` to caller-owned memory; rejected commands,
  nonfinite replies and full buffers fail closed with an explicit error.
  `real_tgp_callback` must be an actual emulator/device transport, **not**
  a placeholder that invents replies.
- **Replay:** initialize with
  `stf_tgp_command_trace_begin_replay(trace, records, count)` and
  bind the same callback. Before returning each recorded response it
  compares the *entire* opcode/argument tuple. A mismatch, truncated
  transcript or invalid reply aborts the path, latches a failure index
  and leaves the reply output unchanged. The previously recovered
  vector code preserves defender X/Y/Z atomically on failure. Check
  `stf_tgp_command_trace_fully_consumed(trace)` to require all
  commands in the captured frame to have been used.
- **Differential:** `stf_tgp_command_trace_compare` returns the first
  divergent tuple index and distinguishes **input**, **reply** and
  **length** mismatches. Reply comparison uses **raw 32-bit equality**,
  not epsilon tolerances. This detects rounding and signed-zero
  regressions that host float approximations would conceal.

Captured sequences can be exported as UTF-8 JSONL, one *observed*
transaction per line. Example record **schema only; the two values
below are illustrative, not extracted from hardware**:

```json
{"opcode":"0x12002424","arguments":["0x00000000","0x40000000"],"reply":"0x00000000"}
```

CLI comparison of captures from an actual STF emulator/device and a
candidate backend:

```sh
python tools/recovery/compare_tgp_command_traces.py \
    traces/reference_tgp.jsonl traces/candidate_tgp.jsonl \
    --json build/tgp_differential.json
```

Exit status is `0` for exact match, `1` for the first differing tuple,
and `2` for malformed/invalid traces. For different finite float reply
words, the report includes **ULP distance** and absolute float delta,
but the match decision remains strictly bitwise. The script refuses
NaNs, infinities and malformed/arbitrary-arity records rather than
silently skipping them. No ROM FIFO address is assumed: unlike a static
code ROM, the TGP responses must be captured **during execution**.

New regressions cover synthetic guard command capture/replay, 4-command
normal-vector replay, command exhaustion, argument mismatches, capacity,
provider failure, NaN responses, bitwise comparisons and the integrated
five-command knockback path. The fixtures exercise the transport and
dispatch contracts **only** and cannot establish 1:1 graphics or physics
fidelity until real Model 2B reply traces are collected and compared.


### Normal hit-sound ROM dispatch in the ordered combat runtime

The previously recovered i960 selector at `loc_2B33C..2B3FC` and
the corresponding ROM sound tables can now be used by the ordered
non-guard attack-hit executor. Opt in with
`recover_normal_hit = true` and `recover_normal_sound = true`;
provide a real `normal_sound_rom` addressable image (setting
`use_schamp_addresses` for Sonic Championship), a callable
`normal_sound_event` audio backend, and its user data.
An invalid configuration is rejected before the first fighter mutation.

After successful CPU combo/damage/reaction recovery, and **before**
the remaining pre-damage hit callback, the runtime runs
`stf_attack_hit_sound_plan_model2` using the transformed damage
result, not the original damage estimate. It resolves every selected
ID using `stf_attack_hit_sound_resolve_rom`, preserving the
correct single-value table family or the bounded zero-terminated
voice-list lookup, according to the two ROM layouts.
`normal_sound_planned`, `normal_sound_resolved` and
`normal_sound_delivered` in the pair result mark the last completed
stage; `normal_sound_plan` and `normal_sound_ids` expose the
specific observed selection.

For a nonempty event, `normal_sound_event` must submit the ID list
to the **real** engine's audio backend. The runtime does **not**
synthesize sound or claim timing fidelity. No audio callback is
issued when the plan returns NONE or when a genuine zero-terminated
list is empty. An invalid ROM table, unresolvable ID or failed
audio callback aborts the remaining vector/tail/damage-unit work;
earlier CPU state is not rolled back. A continuing branch with
`recover_normal_sound` disabled retains the legacy tail-owned
sound behavior. With the opt-in enabled, the tail must **not replay**
the consumed hit-sound event, but still owns unrelated audio/events,
such as ring audio after damage.

The integration regression tests synthetic sfight and schamp sound
table offsets, the normal single-medium tier, character-specific
voice lists, alternate attack-kind table families, a silent list,
attacker-muted hit, post-transform damage tier, missing tables,
rejected sinks, side-exit abort, preflight errors and ordering with
the recovered energy/ring flow. These synthetic rows are test
fixtures, not extracted Sega ROM bytes.

This reproduces the *selection and event contract* seen in the
recovered CPU; callback scheduling and real hardware audio behavior
remain separate 1:1 tasks.


### Guard-block and common-guard audio event delivery

The recovered guard CPU paths now expose an opt-in **real event sink** in
`stf_collision_combat_model2`: set `recover_guard_audio = true`,
`guard_audio_event`, and `guard_audio_event_user_data`. At least one
corresponding `guard_tail` or `guard_common_tail` must be provided.
Missing a sink or both continuations fails preflight before prefix writes.
With this option off, the existing callbacks continue to own audio.

Two different original CPU decisions are consumed:

- Guard `BLOCK_A/B` (the recovered `0x2AA70/0x2AB54` corridor):
  decrement defender `+0xC70` when nonzero, then emit
  `STF_COLLISION_GUARD_AUDIO_KNOCK_9` on a transition to zero or
  `STF_COLLISION_GUARD_AUDIO_KNOCK_3` when it remains nonzero.
  No event is delivered for a zero initial counter. The callback
  executes after guard-state recovery but **before** the five
  optional vector TGP commands and the mandatory guard tail.
  A later TGP or vector-oracle failure cannot undo an emitted sound.
- Continuing common-guard `0x2AC74`: the previously recovered
  `request_sound_cane_2d` is dispatched as
  `STF_COLLISION_GUARD_AUDIO_CANE_2D` after optional
  recovered skill accounting, before `guard_common_tail`.
  Early abort and failed motion lookup do not call the sink.

The pair result reports `guard_audio_requested`,
`guard_audio_delivered` and `guard_audio_kind`; if a real audio sink
returns false, combat stops before remaining device/tail/damage_unit
work, with earlier CPU writes (and, where applicable, score updates)
unchanged. The tail must **not** repeat a consumed audio request.

These three symbols are **CPU-originating sound event types**, not
raw sample IDs or synthesized playable audio. Their audio asset
mapping, playback, and exact Model 2 scheduling remain the host
backend's responsibility. Ordering is defined at the recovered
combat-corridor boundary; it is not yet a bit-/cycle-exact hardware
audio trace. Regression coverage uses synthetic backend callbacks
and validates knock selection, no-sound paths, audio before TGP,
cane after skill, early abort, backend failure and preflight.


### Ordered damage_unit particle requests and sub_32A5C allocation

The ordered combat loop now optionally connects two CPU recovery
corridors that previously required separate calls:

1. `stf_damage_unit_runtime_apply_model2` completes the original
   per-hit `damage_unit` and `damage_unit_post` branches, emitting
   `request_particle_setup`, `particle_draw_kind=1`, and
   `particle_flag_set_mask=1` where the recovered workspace flags
   request a particle.
2. `stf_damage_unit_particle_flow_apply_model2` applies those exact
   staging writes, then calls the recovered `sub_32A5C` allocator
   against the caller-supplied collision-particle RAM slots.

Set `recover_damage_particles=true` and bind
`damage_particle_descriptors` (the real ROM descriptor table),
`damage_particle_slots` and `damage_particle_slots_size` (live
0x24-byte collision-particle slot records), and
`damage_particle_stage` (the caller's staged position vectors and
flags, corresponding to the original 0x50A3FC / 0x50A408 globals).
`also_sub_mode=0x1A` or `0x1B` requires a 32-slot buffer;
all other modes require at least 16 slots. Missing bindings and
undersized buffers are rejected **before the attack prefix mutates
the fighters**. No effect address or asset bytes are fabricated.

After each non-skipped `damage_unit` transaction and its post tail,
the combat loop invokes the particle flow once, before marking the
pair complete. A post tail with no particle request is a no-op and
does not clear the staged position, flags or unrelated draw state.
A skipped `damage_unit` never invokes the particle flow.
A requested particle consumes the staging fields, preserving the
recovered flag-OR behavior; the allocator scans first free slots in
the original order. If the selected pool is full it *successfully*
consumes the request with `allocated=false`, as the recovered
allocator does, instead of forging a new slot or aborting a hit.

The result exposes `pair.damage_particle_processed` and
`pair.damage_particle` (request, allocation flag, slot index and
slot-limit). Errors after the `damage_unit` CPU writes do not roll
back those writes. With the opt-in disabled, the existing
`damage_unit` behavior and caller-owned particle memory are
unchanged. A caller that enables this corridor must not separately
reissue the already-consumed `damage_unit` particle request.

The dedicated CTest
`stf_recovered_collision_damage_particle_combat_tests` validates
single and dual ordered hits, one allocation per eligible request,
the non-request and skipped routes, a full 16-slot pool, special
32-slot access (including slot 16), missing/undersized buffers,
and legacy opt-out. The descriptor addresses in the tests are
**synthetic**, not extracted original ROM asset pointers.

This reconstructs CPU-visible particle allocation and storage.
A graphical emitter, frame rendering, true effects from ROM and
cycle-exact particle timing remain separate tasks.
