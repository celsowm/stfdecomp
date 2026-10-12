# Recovery trace and state contract

The recovery helpers in tools/recovery use a small title-agnostic interchange
format so tracing, structure inference and differential validation do not depend
on a specific emulator or executor.

## JSONL trace records

A memory access record:

    {"type":"memory","step":12,"kind":"read","address":5310848,"size":4,"bytes":"78563412"}

Required fields:

- type = memory
- step: monotonically increasing execution step
- kind: read or write
- address: integer address
- size: access size in bytes

Optional:

- bytes: accessed bytes encoded as hexadecimal
- value: integer value when the tracer has already decoded it

A CPU-step record:

    {"type":"step","step":12,"ip_before":90240,"ip_after":90244}

Required:

- type = step
- step
- ip_before

Recommended:

- ip_after
- instruction word or decoded mnemonic
- call depth when available

Memory and step records may appear in either order as long as they share the
same step identifier. The field inference tool buffers accesses until the step
record is available.

## Differential snapshot JSON

Snapshot producers should prefer stable semantic groups:

    {
      "cpu": {
        "ip": 90240,
        "registers": [0, 1, 2],
        "arithmetic_control": 0,
        "process_control": 0
      },
      "scheduler": {
        "task_index": 3
      },
      "memory": {
        "0x00500020": 1234
      },
      "geometry": {
        "fifo_count": 2
      }
    }

The exact set of keys is milestone-specific. Do not include a field in the
differential contract merely because it is easy to capture. Include it because
the recovery claims to preserve it.

Compare states with:

    python tools/recovery/compare_state.py reference.json recovered.json

Restrict the current contract with repeatable regular expressions:

    python tools/recovery/compare_state.py reference.json recovered.json \
        --include "^cpu\." \
        --include "^memory\." \
        --exclude "frame_counter"

## Fail-closed rule

A recovered path must not silently continue through behavior that has not been
measured. Unsupported paths should remain explicit until a controlled reference
trace or differential experiment establishes their semantics.

## ROM and asset boundary

Trace and state files used for local research may contain observations derived
from a legally obtained ROM, but do not commit ROMs, reconstructed ROM regions,
snapshots containing proprietary bulk data, extracted assets, or large raw
traces to this repository.


## Differential impact vectors after TGP replay

The command transcript comparator answers whether the same requests and
reply words were seen. To verify their **observable effect**, use
`stf_tgp_command_trace_compare_vectors(reference_xyz_bits,
candidate_xyz_bits, &difference)` from
`src/recovered/tgp_command_trace.h` once both executions have finished
the original CPU multipliers and stored the fighter's X/Y/Z impact at
`+0x5E0/+0x5E4/+0x5E8`.

The result's `mismatch_mask` marks X (bit 0), Y (bit 1), and Z (bit 2)
that differ **bitwise**. The `ulp_distance[3]` members measure
ordered IEEE-754 binary32 distance for finite values, including negative
numbers; signed zeros have separate bit patterns and are one ordered
ULP apart. A NaN or infinity is rejected, not treated as matching.
This method supplies a numeric difference report, not permission to
accept approximate results as hardware-equivalent: the strict 1:1
criterion remains bit equality on all three axes, on matching inputs.

The TGP capture callback uses caller-provided actual device outputs;
synthetic unit-test values must never be mislabeled as Model 2 reference
capture evidence. Do not commit proprietary ROM bytes or raw captures.


## Strict impact-vector oracle in ordered combat

The recovered Model 2 combat runtime supports **opt-in fail-closed final
vector verification**, after the ROM-backed `calc_mht_adr` continuation and
hardware TGP replies, but **before** writing the defender's impact fields.

To enable it, set `recover_normal_hit = true`,
`recover_normal_knockback = true`, provide the normal ROM view and
`normal_knockback_tgp` callback (an actual TGP device, or a strictly
validated command/reply replay), then set
`verify_normal_knockback_vectors = true`. Supply
`normal_knockback_expected_xyz[defender_index][0..2]` as three raw
IEEE-754 **X/Y/Z words** captured independently from the original
execution **after** its CPU multipliers. A reference from the same
recovered implementation is useful as a unit-test fixture but **does
not establish** original hardware fidelity.

The opt-in checks both defender slots for finite reference values
before the first combat state mutation and rejects configurations
without the actual TGP callback; approximate host trigonometry cannot
validate a bit-exact reference. On generic/down routes requiring the
shared motion vector, the recovered runtime computes the complete
candidate in temporary storage and compares it with the independent
reference using `stf_tgp_command_trace_compare_vectors`. If every
word matches, it sets `normal_knockback_vectors_checked` and commits
X/Y/Z as usual.

A mismatch reports `normal_knockback_vector_difference.mismatch_mask`
and `ulp_distance[3]` in the pair result, sets
`normal_knockback_vectors_checked = true`, and returns failure
**without overwriting any fighter vector component**, calling the
remaining attack tail, or running `damage_unit`. Combo, reaction and
other earlier writes are explicitly **not** rolled back. Hardware
reply failures or invalid computations abort before comparison and
leave `normal_knockback_vectors_checked = false`.

The option only checks those generic/down routes that actually emit
the recovered vector; normal/KO/special branches leave the flag
unset rather than manufacturing proof of equivalence. The caller
still needs to check the strict command transcript's
`stf_tgp_command_trace_fully_consumed` on replay to detect trailing
unconsumed recorded commands. A successful oracle check proves
equality for the supplied references and inputs, not global 1:1
game fidelity.

Regression scenarios cover an exact three-axis match, a 1-ULP
divergence, NaN reference rejection before side effects, missing
TGP provider, route skip and interrupted replay.
