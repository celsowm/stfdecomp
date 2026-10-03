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

The runner also auto-attaches `rom/rom_data.bin` and `rom/rom_ep.bin` when
those locally extracted files exist. They are mapped read-only at their
evidence-backed Model 2B data-ROM windows. No ROM data is stored in a scenario.

Optional fields:

- `data_rom`: override the optional main-data blob (default: `rom/rom_data.bin`);
- `ep_rom`: override the optional EP blob (default: `rom/rom_ep.bin`);
- `stop`: address or symbol;
- `sat`, `prcb`, `reset_from_prcb`;
- `call_entry`: enter the selected function through a real i960 procedure
  frame so its `ret` can complete;
- `return_address`: override the synthetic return sentinel used by
  `call_entry`;
- `registers`: object such as `{"g0":"0x1234"}`;
- `work_ram_in`, `work_ram_out`;
- `trace`, `state`;
- `probe.allow_write`: explicit device-write ranges;
- `probe.stub_read`: explicit device read stubs.

Attaching data ROMs is not exploratory: they are read-only source data.
Any scenario that accepts or stubs unknown hardware is exploratory. The
underlying corridor runner records that state as not suitable for reference
comparison.

## Templates

Files ending in `.template.json` deliberately contain `"template": true`.
The scenario runner refuses to execute them. Copy one, fill only values backed
by an STF trace/reference observation, then remove the template flag.

Do not commit ROMs, large work-RAM dumps or proprietary extracted data.


## Executable scenarios

Two scenarios are intentionally runnable as-is once the local ROMs/tools exist.

Strict boot:

    make recovery-boot

This starts at address zero and stops at the first still-unmodeled Model 2B
device access. It uses no probe allowances.

Isolated collision-task initializer:

    make recovery-coli-init

This uses the mechanically extracted `fa_coli` descriptor
(workspace size `0x400`, init `coli_init`) and the task ABI observed in the
scheduler, where `g13` points at the active module workspace. The scenario
places that workspace at `0x005ff000`, uses a separate stack, enters through
an architectural call frame, and dumps work RAM for inspection. It does not
pretend to reproduce a live fight state.

`camera_init` remains a template because it quickly depends on fighter state
and the still-unmodeled coprocessor path.
