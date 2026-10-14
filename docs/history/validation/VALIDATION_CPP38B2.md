# Validation — cpp38b2 FPU_METRICFREE_RECOVERY

## Source / ABI

- `VERIFY_CPP38B2.py`: **19/19 PASS**.
- Guest ABI v1 structural fingerprint: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f` — MATCH.
- Historical SAFE ChuChu seed remains unchanged (`ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4`) but cpp38b2 intentionally does not auto-adopt it because it predates the metric-free generated implementation.

## Build / regression

- Linux Release top-level build: PASS.
- Fast project regression: **52/52 PASS**.
- `generated_single_build`: PASS (21.10 s in this environment).
- `generated_program_build`: PASS (20.14 s in this environment).
- Effective suite: **54/54 PASS**.
- Runtime + network + Maple + GD-ROM Linux Release build: PASS.

## Metric-free guest fixture

A generated FPU fixture was modularized with the cpp38b2 Guest ABI and rebuilt with:

`DCR_FIXED_FPU_REGION_METRICS=0`

Result: `generated_compile_test` PASS. The generated source contains the new `DCR_FPU_METRIC_INC/ADD` gates and the modular target receives the fixed-off definition.

The cached-source migration helper was also tested against representative old Superblock/scalar counter patterns and rewrote them to the metric macros without changing SH-4 behavior.

## Windows launcher

- Cross-compiled Go GUI x64 launcher: PE32+ Windows GUI.
- Launcher v1.4 SHA-256: `034d725baca1ed080cd80b54be7546ef235df77b67562dbf9d29bd0b109125e0`.

## Expected Windows heartbeat

```text
build=v1.2-cpp38b2-fpu-metricfree-recovery
fpu-mode=superblock
fpu-metrics=off
host-sync-q=262144
pvr38=on/cpp38b2/indexscratch/capture1copy/ch2bulk/simdtag/homstrip
```

Because production FPU telemetry is compiled out of the migrated guest, `fpu-super`, `fpu-cache` and related diagnostic counters are expected to stay at zero instead of proving activity through counter growth. `fpu-mode=superblock` is the execution-mode indicator for this A/B build.
