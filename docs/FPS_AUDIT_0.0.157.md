# FPS audit — 0.0.157

## Evidence from 0.0.156 live A/B

Three ChuChu Rocket! runs compared PERF+hottrace, normal+hottrace and no-hottrace. The no-hottrace executable was consistently faster at matched PVR/FPU workload, so adaptive trace promotion is removed from normal 0.0.157 code generation rather than merely disabled at runtime.

## Flycast audit applied to 0.0.157

Flycast's SH-4 dynarec first versions registers in SSA, performs constant/dead-code/dead-register simplification, then its register allocator preloads live sources, allocates destinations without an unnecessary preload, tracks dirty/writeback state, and flushes at architectural barriers. Its x64 backend uses allocated GPRs directly for PREF when available.

DreamcastRecomp cannot directly reserve physical x64 registers from portable emitted C++, but can expose the same shape to MSVC: compile-time liveness/use analysis, scalar locals, no dynamic valid masks, source-only preload, internal-edge persistence, barrier writeback and exact fallback for unsafe functions.

## 0.0.157 changes

- Static GPR cache: R0-R15, PR, GBR, MACH, MACL, SR/T.
- No adaptive hot-trace scaffolding in normal generated code.
- Scheduler-safe flush/reload at the existing 256-cycle full-tick boundary.
- Call/SR/exit barriers materialize architectural state before external observation.
- `DCR_GPR_CACHE=0` is a recompiler-time A/B switch only.
- Proven fused SQ packets build directly in `PVRTAStreamRaw` and move into TA staging after the architectural SQ copy, removing the extra packet-to-stage memcpy.
- PERF-only `gpr-cache=` and `sq-zero=` counters avoid normal-gameplay bookkeeping cost.

## Next candidates after live 0.0.157 measurement

1. Profile actual GPR-cache coverage / static loads / writebacks in Mouse Mania.
2. If SH-4 remains dominant: stronger SSA constant/dead-register propagation before C++ emission and split hot/cold functions to improve MSVC register allocation.
3. Specialized x64/SIMD code generation for the proven FTRV/FIPR hot kernels without changing Flycast-compatible arithmetic semantics.
4. More direct TA decode for safe fused packets if staging parse remains material.
5. MSVC LTCG/PGO only after code-shape improvements are measured, because commercial compile time is already high.
