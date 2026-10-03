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
