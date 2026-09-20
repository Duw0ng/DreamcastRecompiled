# DreamcastRecomp 0.0.107 validation

## Stable policy

- 0.0.94 remains the original frozen rollback baseline.
- 0.0.106 is promoted to the current stable functional release after live CDDA validation.
- 0.0.107 begins the next performance branch.

## Commercial compile scalability

The previous retail backend emitted the entire SH-4 closure into one very large `generated_program.cpp`, forcing the host optimizer to process almost 3,000 functions as a single translation unit. The commercial rebuild BAT also deleted the entire generated/build directory on every run, defeating incremental compilation.

0.0.107 changes this without changing SH-4 semantics:

- Retail closures with at least 256 functions are divided into up to 32 address-local, work-balanced C++ translation units.
- `generated_program.cpp` is retained as a small registration TU for compatibility.
- CMake lists the generated shards explicitly.
- MSVC uses `/MP8`, `/O2`, `/Oi`, `/Ot`, `/Gy`; IPO/LTO remains disabled.
- Windows BAT builds are capped at 8 parallel jobs to avoid excessive compiler-memory pressure.
- Codegen writes a generated file only when its bytes changed, preserving timestamps and object-cache validity.
- `run_commercial_recompile.bat` preserves `generated/commercial_recompiled/cpp/build` by default. Set `DCR_CLEAN_RECOMPILE=1` only when a clean commercial rebuild is explicitly wanted.

## Exact ChuChu Rocket closure

Using the same commercial CDI used for previous validation:

- Synthetic/reachable functions: 2,965
- Reachable SH-4 instructions: 295,215
- Known SH-4: 295,215
- Unknown SH-4: 0
- RAW_SH4: 0
- CFG blocks: 44,153
- DCIR ops: 301,347
- Generated retail shards: 32
- Shard sizes: roughly 0.5–3.0 MiB; most are around 1.1–2.7 MiB
- `generated_program.cpp` now mainly contains registration rather than all function bodies.

A second identical commercial emission changed the timestamps of **0 / 45** generated top-level build files. A completed incremental Ninja build immediately afterwards reported `no work to do` and returned in about 0.02 s on the validation host.

The generated multi-TU commercial runner and `generated_compile_test` were compiled and linked successfully in the validation environment; `generated_compile_test` returned 0.

## Core regression suite

- 47 / 47 CTest PASS.

## Runtime profiling

`run_commercial_recompiled_perf.bat` now profiles the same principal execution path as the normal runner: D3D11 PVR GPU, PVR MT, fast/direct dispatch, SH-4 tick batching, host-paced AICA and host75 CDDA. This avoids profiling a software-rendering configuration that is not representative of normal gameplay.
