# Recovery scenarios

Scenario files make STF i960 recovery runs reproducible without hard-coding
function addresses into shell scripts.

Run one with:

    python tools/recovery/run_scenario.py tools/recovery/scenarios/my-run.json

The scenario runner resolves `entry` and `stop` through an nm960/GNU-nm
symbol listing when strings are used.

## Minimal schema

    {
      "name": "camera-init",
      "rom": "rom/rom_code1.bin",
      "symbols": "build/rom_code1.nm",
      "entry": "camera_init",
      "steps": 50000,
      "stack": "0x005ff800"
    }

Optional fields:

- `stop`: address or symbol;
- `sat`, `prcb`, `reset_from_prcb`;
- `registers`: object such as `{"g0":"0x1234"}`;
- `work_ram_in`, `work_ram_out`;
- `trace`, `state`;
- `probe.allow_write`: explicit device-write ranges;
- `probe.stub_read`: explicit device read stubs.

Any scenario that accepts or stubs unknown hardware is exploratory. The
underlying corridor runner records that state as not suitable for reference
comparison.

## Templates

Files ending in `.template.json` deliberately contain `"template": true`.
The scenario runner refuses to execute them. Copy one, fill only values backed
by an STF trace/reference observation, then remove the template flag.

Do not commit ROMs, large work-RAM dumps or proprietary extracted data.
