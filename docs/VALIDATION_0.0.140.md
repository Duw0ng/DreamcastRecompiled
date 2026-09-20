# Validation 0.0.140

Status: experimental. Stable rollback remains 0.0.131.

## Architecture under test

- Based on the 0.0.137 runtime.
- Commercial TA submission captures each 32-byte packet into a contiguous scene stream.
- A small front FSM tracks 32/64-byte continuation/list state and signals End-Of-List immediately.
- Full parser replay occurs before TA_LIST_INIT reset and before ISP_START rendering.
- Consecutive ordinary 32-byte vertices are consumed by a direct bulk vertex loop with the specialized decoder already selected.
- Packet-limit probes retain immediate parsing.

## Automated validation

- Main Release build: PASS, strict `--parallel 1`.
- CTest: 50/50 PASS.
- Fresh generated C++ runtime: PASS.
- `generated_compile_test`: RC=0.
- Fresh generated runner: RC=0.
- Staging self-test: PASS. It verifies zero decoded vertices before replay, six captured packets for header+4 vertices+EOL, immediate EOL IRQ, four bulk-decoded vertices after replay, at least one bulk run, empty stream after parse, and no duplicate IRQ.

## Live telemetry to inspect

`pvr-ta-stage=C/F/R/V/G/P`

- C: packets captured
- F: staged parse flushes
- R: direct bulk runs
- V: vertices consumed by bulk path
- G: packets replayed by generic/immediate parser
- P: maximum staged packets in one pending stream

`pvr-ta-stage-ms` records cumulative staged parse time in diagnostics.

The main live comparison should be against 0.0.137 under matched packet/triangle load.
