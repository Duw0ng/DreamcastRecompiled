# DreamcastRecomp 0.0.132 validation

- Accepted stable baseline: **0.0.131**.
- Change scope: TA parser CPU-only micro-optimizations; no timing, renderer semantics, audio, GPU state, or build-policy changes.
- Linux Release core build: PASS.
- CTest: **47/47 PASS**.
- Fresh generated runtime (`dc_recomp_memory_output`) configure/build: PASS.
- `generated_compile_test`: **RC=0**.
- Fresh emitted `dreamcast_program`: **RC=0**, returned normally to host (`R0=150`).
- Emitted heartbeat contains `pvr-ta-fast=vtx7/le32/src4/v64cache`: PASS.
- Generated CMake contains no active MSVC `/MP` option: PASS.
- Commercial Windows build script remains `--parallel 1`, `CL_MPCount=1`: PASS.
- `CURRENT_STABLE.txt` remains **DreamcastRecomp 0.0.131**.
- Type-7 dispatch equivalence: PASS for all 16 possible PCW top nibbles; `hdr_type==7` is exactly the old `command==0xE... || 0xF...` set.
- Package ZIP integrity: **PASS**.
