# SGGG cpp17 checkpoint — real menu reached

This checkpoint preserves the first confirmed real Segagaga menu milestone.

## Analyzer/runtime changes after cpp16

- Medium anchored callback-table recovery was generalized for 8–16 entry Katana tables.
- Such tables require at least four already-proven members and at least one third of the confidence window to be anchored.
- Every newly promoted member still has to pass an independent callable-entry validator.
- This structurally recovers SGGG `0x8C182864` from the 14-entry callback table around `0x8C2073BC`; no title-specific seed was added for it.

## Static closure

- Reachable functions: **6,393**
- Reachable SH-4 instructions: **676,914**
- Known SH-4: **676,914**
- Unknown SH-4: **0**
- RAW_SH4: **0**
- `0x8C182864` is present as an independent native function-map entry.
- This closure exceeds the historical cpp10 closure (6,391 functions / 676,901 instructions).

## Validation

- Full project CTest: **54/54 PASS**.
- Generated runtime build: **42/42**, RC=0.
- `generated_compile_test`: **RC=0**.
- Clean visual run without START probe reached 4,780 PVR frames / 4,753 flips without a missing target or SH-4 crash.
- Diagnostic run with `--probe-controller-start-burst` delivered real low-level Maple GETCOND START pulses beginning after frame 1,800.
- The START pulses skipped the long intro and reached the actual **Sega Simulation SEGAGAGA** menu.
- Captured menu options: **New Game / Continue / View Archive / Options**.

## Included evidence

- `SGGG_CPP17_MENU.png` — confirmed real menu framebuffer.
- `SGGG_CPP17_RUNTIME_MENU.log` — runtime log from the successful menu run.
- `SGGG_CPP17_FUNCTION_MAP.csv` — complete function map.
- `SGGG_CPP17_ADDRESS_TAKEN_EVIDENCE.csv` — address-taken evidence map.
- `SGGG_CPP17_CLOSURE.txt` — closure summary.
- `SGGG_CPP17_CTEST.txt` — full 54-test result.
- `SGGG_CPP17_SOURCE_CHANGES.patch` — source/profile delta from cpp16.

## Important diagnostic note

The automatic START pulses are opt-in through `--probe-controller-start-burst`. They are diagnostic and are not part of normal/manual input behavior.

The user-owned SGGG CDI is intentionally not included in this checkpoint.
