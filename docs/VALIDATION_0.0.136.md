# DreamcastRecomp 0.0.136 validation

Status: experimental. Stable rollback remains 0.0.131. 0.0.135 remains the previous experimental checkpoint.

## Change

0.0.136 extends the guarded native TA PREF producer analysis from the local
same-basic-block heuristic introduced in 0.0.135 to a whole-function CFG
may-dataflow analysis.

Producer provenance:
- originates when a GPR is used as the base of Store/FMOV/MOVCA memory writes;
- crosses direct and conditional CFG edges;
- propagates through `MOV Rm,Rn` aliases;
- survives conservative address adjustments such as ADD/SUB/AND/OR;
- is killed by definite GPR replacement;
- is cleared across calls, opaque `RawSH4`, and exception/RTE boundaries.

A flow-recognized PREF uses `dc_pref_ta_native(..., true)`. The runtime still
validates that the live PREF address is a Store Queue aperture and that the
current QACR maps it to the TA FIFO. Any mismatch falls back to the original
`dc_pref()` path.

## Diagnostics

- `pvr-ta-fast=vtx7/le32/src4/v64cache/specdec/nativepref/cfgflow`
- `pvr-ta-native=<all hits>/<all fallbacks>`
- `pvr-ta-flow=<local hits>/<local fallbacks>/<CFG hits>/<CFG fallbacks>`

## Validation

- Main project Release build: PASS, strict serial (`--parallel 1`).
- CTest: 49/49 PASS.
- Emitter tests verify:
  - local Store32 -> PREF remains a native candidate;
  - Store32 in one block -> branch -> MOV alias -> PREF in another block emits
    `dc_pref_ta_native(..., true)`;
  - a definite `MOV #imm` overwrite kills producer provenance and preserves the
    generic `dc_pref()` path.
- Fresh generated runtime: PASS.
- `generated_compile_test`: RC=0.
- Fresh generated `dreamcast_hello`: RC=0 and returns normally to host.
- Generated CMake: no active MSVC `/MP`; serial/low-memory policy retained.
