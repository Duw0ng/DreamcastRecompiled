# DreamcastRecomp 0.0.131 validation

- Baseline: 0.0.130 runtime + strict serial Windows commercial build.
- Change scope: TA strip rolling window becomes a 3-slot ring; duplicate vertex plausibility checks removed.
- Ring equivalence: old shift and new ring produced identical triangle triples for mixed strips of lengths 1 through 30.
- Linux Release core build: PASS.
- CTest: 47/47 PASS.
- Fresh generated runtime (`dc_recomp_memory_output`) configure/build: PASS.
- `generated_compile_test`: RC=0.
- Windows build policy preserved: generated CMake has no active MSVC `/MP`; commercial build script remains `--parallel 1`.
