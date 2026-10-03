# VF2 -> Sonic the Fighters cross-title recovery

This document tracks evidence that the Sonic the Fighters Model 2B program
inherits runtime conventions also recovered independently in
celsowm/vf2-decomp for Virtua Fighter 2 on Model 2A.

The purpose is not to assert that both games use identical functions. The
purpose is to turn VF2 recovery work into high-value hypotheses that can be
tested against STF instead of repeating reverse engineering from zero.

## Confirmed shared runtime anchors

The following addresses are present in the STF linker map and correspond to
runtime locations already used by recovered VF2 code.

| Address | STF symbol | VF2 recovered meaning | Confidence |
| --- | --- | --- | --- |
| 0x020E0004 | POLYGON_NUM_OFFSET | polygon/object table base used by polygon submission | high |
| 0x00501004 | BUFF_ADD | geometry command/buffer state | high |
| 0x00501008 | BUFF_MAX | geometry maximum/buffer metric | high |
| 0x00501018 | POLYGON_LIMIT | polygon/geometry submission gate | high |
| 0x0050101C | POLYGON | polygon/geometry accumulator/gate | high |
| 0x00501084 | focus_dist_x | horizontal focus/camera scale | high |
| 0x00501088 | focus_dist_y | vertical focus/camera scale | high |

The 0x020E0004 match is especially useful because VF2's recovered
polygon_object_submit path indexes records from that exact address while STF
names the same location POLYGON_NUM_OFFSET.

These matches are evidence of a shared or evolved AM2 Model 2 runtime layer.
They are not by themselves proof that an entire containing function is the
same across Model 2A and Model 2B.

## Recovery strategy

Use three layers and keep them separate:

1. Shared recovery tooling
   - i960 decoding and control-flow analysis
   - linker symbol normalization
   - memory-access tracing
   - snapshot/probe scenarios
   - candidate structure inference
   - differential validation

2. Cross-title evidence
   - stable Model 2 RAM/MMIO/table addresses
   - call and branch topology
   - FIFO traffic shapes
   - relative fighter/object offsets
   - geometry command sequences

3. STF-specific semantics
   - Sonic fighter state
   - motion tables
   - SKELETON_TYPE_DATA and CHAR_PARTS
   - osage/secondary motion
   - STF collision and damage rules
   - camera modes, stages and replay behavior

Never promote layer 2 evidence directly into layer 3 semantic names without a
controlled STF observation.

## Cross-title matcher

Run the matcher with sibling checkouts:

    python tools/recovery/model2_crossmatch.py \
        --vf2-root ../vf2-decomp \
        --stf-root . \
        --min-shared 2 \
        --top 5 \
        --format markdown \
        --output out/vf2-stf-crossmatch.md

JSON output is available for later automation:

    python tools/recovery/model2_crossmatch.py \
        --vf2-root ../vf2-decomp \
        --stf-root . \
        --format json \
        --output out/vf2-stf-crossmatch.json

The matcher:

- reads VF2 recovered C and bounded hardware-model C;
- reads STF assembly labels;
- resolves STF symbolic operands through the linker scripts;
- ignores ordinary code addresses and prioritizes Model 2 RAM/MMIO/table
  addresses;
- weights rare shared addresses more strongly;
- gives extra weight to the already confirmed anchors above;
- emits candidates rather than renaming anything automatically.

## First high-value cross-title targets

### Geometry submission

VF2 already has recovered code for:

- polygon object submission;
- geometry command setup;
- frame geometry commit;
- TGP FIFO handling;
- polygon ROM access;
- matrix/focus state.

STF already exposes the same core geometry globals, so geometry submission is
the first subsystem to cross-match.

Success criterion:

- find one or more STF assembly regions sharing several of the confirmed
  geometry anchors;
- then verify their FIFO/store behavior by tracing before naming them as
  descendants of VF2 routines.

### Fighter pose / skeleton

VF2 has a measured 16-joint pose corridor and recovered joint tuple exchange
with the geometry coprocessor.

STF exposes:

- SKELETON_TYPE_DATA;
- CHAR_PARTS;
- rob_info_skeleton_type;
- rob_info_mot_kind;
- rob_info_mot_num;
- rob_info_mot_coma;
- motions and osage-related symbols.

The VF2 joint path should therefore be used as a protocol hypothesis, not
copied as implementation.

Success criterion:

- identify STF routines that emit similar coprocessor traffic around fighter
  pose update;
- correlate selector/index values with STF skeleton and motion state;
- only then define an STF pose structure.

### Collision / fighter layout

VF2 recovery already contains fa_coli research and fighter-relative memory
access analysis.

For STF, normalize observed addresses as:

    relative_offset = address - fighter_base

Compare access sequences and widths, not only absolute offsets. Structure
layouts may have grown or moved between titles.

Success criterion:

- identify repeated bilateral offsets for both fighters;
- cluster read/write roles;
- correlate with coli_init and debug collision symbols;
- preserve neutral field names until behavior is independently proven.

### Sound

VF2 has reusable methodology and bounded models for:

- 68000 inspection;
- SCSP register access;
- command rings;
- voice maintenance;
- stream descriptors.

STF sound executable decompilation is still incomplete upstream. Reuse the
analysis workflow, but do not assume binary-compatible command layouts.

## Probe and differential direction

The VF2 project proves a useful recovery contract:

    original i960 execution
             |
             +---- reference state
             |
        recovered C
             |
             +---- recovered state
             |
        compare at controlled boundary

For STF the same architecture should eventually compare, as applicable:

- i960 registers;
- condition state;
- local call frames;
- mutable work RAM;
- scheduler/task state;
- modeled Model 2B device-visible state;
- selected FIFO/geometry state.

Unknown branches must fail closed. A candidate cross-title match is not enough
to make an unsupported path succeed.

## Legal boundary

The upstream STF repository states that it is source-available and carries no
open-source license grant. Keep this fork's recovery work cleanly separated
from ROM data and proprietary extracted assets.

The intended port path remains:

    user-supplied original ROMs
      -> local extraction/recovery tools
      -> normalized semantic data
      -> Saturn-specific conversion/runtime

Do not commit ROMs, reconstructed proprietary regions, extracted models,
textures, music, or other ROM-derived payloads.
