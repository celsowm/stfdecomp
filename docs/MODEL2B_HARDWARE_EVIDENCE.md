# Model 2B hardware evidence and recovery boundary

This document records what the STF host recovery runtime may model by default
and what must remain fail-closed until Sonic the Fighters-specific evidence
establishes behavior.

The implementation lives in:

    tools/recovery/i960/src/model2b_bus.c
    tools/recovery/i960/src/model2b_map.c

This is not intended to become a full Model 2 emulator. Its purpose is to run
controlled i960 corridors and stop exactly where unknown Model 2B behavior
begins.

## Evidence levels

- **STF-direct**: address/use is present in this repository's linker map,
  assembly, task tables, or extracted-ROM manifest.
- **MAME-corroborated**: current MAME Model 2B mapping independently agrees
  with the address or storage role.
- **Raw storage**: byte-addressable RAM behavior is sufficiently established
  to model without assigning higher-level device semantics.
- **Read-only data**: user-supplied local extracted ROM is attached at an
  evidence-backed address.
- **Device / fail-closed**: address is known, behavior is not; access returns
  STF_ERROR_UNSUPPORTED unless an explicit exploratory probe policy handles it.

## Default map

| Address/range | Evidence | Default recovery behavior |
| --- | --- | --- |
| 0x00000000... | STF linker program-ROM origin | attached program ROM, read-only |
| 0x00500000-0x005fffff | STF linker work RAM | raw RAM |
| 0x00800000-0x00803fff | GEO_START | **device / fail-closed** |
| 0x00804000-0x00807fff | GEO_PROGRAM_START | **device / fail-closed** |
| 0x00880000-0x00883fff | STF startup reconstructs g11=0x00880000 | **device / fail-closed** |
| 0x00884000-0x00887fff | STF startup uses g11 + 0x4000 | **device / fail-closed** |
| 0x008c0000... | COPRO_SHARC_IOP_START | **device / fail-closed** |
| 0x00900000-0x0091ffff | BUFF_RAM_START and four STF bank labels | raw buffer RAM |
| 0x00980000... | copro/geometry/video control labels | **device / fail-closed** |
| 0x00e00000-0x00e00037 | CPU_CONTROL_START; startup copies wait data here | raw storage |
| 0x00e80000... | IRQ request/enable | **device / fail-closed** |
| 0x00f00000... | timers | **device / fail-closed** |
| 0x01000000... | tile/video windows | **device / fail-closed** |
| 0x01800000-0x01803fff | stage/polygon palette window | raw storage only |
| 0x01810000-0x0181bfff | COLORXLAT_START | raw storage only |
| 0x01c00000... | STF I/O ports | **device / fail-closed** |
| 0x01c80000... | SERIAL_START | **device / fail-closed** |
| 0x01d00000-0x01d03fff | BACKUP_RAM_START | raw RAM, initialized to 0xff |
| 0x02000000... | rom_data.bin / main-data bus | attached read-only data ROM |
| 0x03000000... | rom_ep.bin | attached read-only EP ROM |
| 0x06000000... | MAME-corroborated EP mirror | same attached EP ROM, read-only |
| 0x10000000... | render-mode/display window | **device / fail-closed** |
| 0x11000000-0x111fffff | texture RAM 0 | raw storage only |
| 0x11200000-0x113fffff | texture RAM 1 | raw storage only |
| 0x11400000-0x1140ffff | luma RAM | raw storage only |

Raw palette/texture/luma storage does **not** imply that rendering semantics are
recovered. It only allows corridors that copy or inspect bytes to continue.

## Direct STF observations

### Geometry and coprocessor addresses

The STF startup constructs:

    g10 = 0x00800000
    g11 = 0x00880000
    g12 = 0x00004000

Therefore an access through g11 + g12 reaches 0x00884000. The code uses this
path while loading the coprocessor program and in later object/collision paths.

The assembly also contains the command word:

    0x1A003434

inside set_obj_tpd, matching a command observed independently during VF2
recovery. That is strong cross-title evidence, but it is still not sufficient
to emulate the command or synthesize its response.

For that reason all GEO, TGP/SHARC FIFO/function ports, and their control
registers remain devices by default.


### Recovered scalar square-root command

The command word:

    0x0D001A1A

appears six times in the STF program with a one-word float input and one-word
float output. Multiple independent call sites constrain its semantics:

- `calc_land_time` forms a value equivalent to `v^2 + 2gh`, sends it through
  the command, then uses the returned value in the standard landing-time form
  `(sqrt(v^2 + 2gh) - v) / g`;
- `attack_hit` converts an 8-bit strength value to float, multiplies by
  `0.01f`, sends that non-negative scalar through the command, then applies
  further scalar multipliers;
- four other call sites use the same one-input/one-output scalar pattern.

This is sufficient to recover the semantic operation as scalar square root.
The portable implementation lives in:

    src/recovered/copro_scalar.c

Confidence: **high, STF-direct semantic recovery**.

Important boundary: this does **not** make the whole 0x00880000/0x00884000
coprocessor transport understood. The default Model 2B bus remains fail-closed
for that device range. The recovered helper models only the proven scalar
operation and must not be read as a general TGP/SHARC emulator.

### Recovered scaled sine/cosine commands and attack-hit vector

The cpres1 command words used immediately after the recovered `attack_hit`
motion prefix are:

    0x12002424
    0x12802525

STF call sites consistently send a 16-bit angle plus one float scale and read
one float result. The STF cpres1 firmware analysis published with
`biggestsonicfan/m2-sdk` identifies the corresponding operations as:

    0x24: sin(angle) * scale
    0x25: cos(angle) * scale

The command encoding is `(N << 23) | (N << 8) | N`, and angles use one full
turn over 0x10000 units.

That closes the next `attack_hit` frontier. In `loc_2B7E0..loc_2B898` the
program constructs the defender knockback vector from the prefix magnitude:

    y          = sin(angle_r6) * magnitude
    horizontal = cos(angle_r6) * magnitude
    x          = sin(-r10) * horizontal
    z          = cos(-r10) * horizontal

It then applies the observed hit-mode/profile multipliers and the
`flt_2B904` attacker-state scale table before storing the three components at
defender offsets `+0x5E0/+0x5E4/+0x5E8`.

The portable semantic recovery lives in:

    src/recovered/copro_scalar.c
    src/recovered/attack_hit_motion_vector.c

The reference ROM corridor `[0x0002B738,0x0002B898)` is pinned by
`tools/recovery/validate_sfight_rom.py` with SHA-256:

    689813368538becefc91d2de834e4e51c952c4724ca2aee6dabebb336090479f

Confidence: **high, STF-direct usage plus STF cpres1 firmware corroboration**.

As with the square-root helper, the portable trigonometric helpers model the
recovered operation, not the full FIFO/device transport and not a claim of
bit-identical reproduction of the cpres1 lookup table. The Model 2B device
range therefore remains fail-closed by default.

### Buffer RAM

STF declares:

    BUFF_RAM_START = 0x00900000
    BUFF_RAM_01    = 0x00908000
    BUFF_RAM_02    = 0x00910000
    BUFF_RAM_03    = 0x00918000

The recovery runtime exposes only the declared 0x20000-byte window as byte
storage. It does not currently add undocumented mirrors or geometry side
effects.

### CPU-control storage

start_ip copies the wait-state/control table into CPU_CONTROL_START at
0x00e00000. MAME also maps the small CPU-control region as RAM-like storage.
The recovery runtime therefore exposes the observed 0x38-byte window as raw
storage, without interpreting the fields.

### Main-data ROMs

tools/sfight_data.json constructs:

- rom_data.bin from the main data ROM set;
- rom_ep.bin from the EP pair.

The runner can attach these locally extracted user-supplied files as read-only
data. run_corridor.py and run_scenario.py automatically attach the conventional
rom/rom_data.bin and rom/rom_ep.bin paths when they exist.

### I/O and serial

STF writes to 0x01c000xx and 0x01c80000 during startup. The current MAME
Model 2B driver does not provide enough corroborated STF-specific behavior to
treat those writes as understood, and sfight remains marked MACHINE_NOT_WORKING
there.

Those ranges therefore stay fail-closed. A controlled experiment may allow
specific observed writes explicitly, but the resulting state is exploratory.

## MAME use in this project

The external reference inspected for address-map corroboration was:

    mamedev/mame
    src/mame/sega/model2.cpp
    master observed at a2b6ba2d4be70dabf7ff7a642749dda0c6e70498

MAME is used as independent evidence, not as permission to copy a full Model 2B
device model into this recovery runtime. When STF evidence and MAME differ or
MAME is incomplete for sfight, STF execution remains fail-closed.

## Exploratory probing

Build the host recovery tools:

    make recovery-test

Extract local ROM data:

    python tools/data_extract.py --rom

Run a strict corridor:

    build/recovery-i960/stf_i960_corridor \
        --rom rom/rom_code1.bin \
        --data-rom rom/rom_data.bin \
        --ep-rom rom/rom_ep.bin \
        --entry 0x00000000 \
        --steps 100000 \
        --trace out/boot.jsonl \
        --state out/boot-state.json

Unknown hardware stops execution and reports the first address, access size,
direction, region hint, and exact linker-derived symbol when available.

A measured write can be temporarily allowed:

    --allow-write 0x01c80000:0x01c80004

A measured read can be replayed:

    --stub-read 0x00884000=0x12345678

Neither option asserts hardware semantics. Any run that consumes such a policy
is marked exploratory and must not be treated as a reference recovery result.

## Evidence loop

Use:

    run -> first unsupported access -> classify -> gather STF/reference evidence
        -> model only proven behavior -> rerun -> differential comparison

Aggregate first faults with:

    python tools/recovery/summarize_faults.py out/recovery/*.jsonl

Analyze traces with:

    python tools/recovery/classify_tgp_trace.py TRACE.jsonl
    python tools/recovery/trace_fields.py TRACE.jsonl --base NAME=ADDRESS

Compare controlled snapshots with:

    python tools/recovery/compare_state.py reference.json recovered.json

## Next device priorities

The next default device behavior should be selected by actual first-fault
frequency across reproducible STF scenarios, not by completeness ambitions.

Likely categories are:

1. startup serial/I/O writes;
2. IRQ/timer behavior required for deterministic scheduling;
3. input reads for controlled fighter scenarios;
4. TGP/SHARC request/response behavior, initially from captured pairs;
5. sound-board communication after its traffic is measured.

Full SHARC execution is not a prerequisite for useful semantic recovery.
