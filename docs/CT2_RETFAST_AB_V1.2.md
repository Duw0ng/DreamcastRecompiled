# DCR V1.2 — Crazy Taxi 2 SH-4 trivial-return A/B test

Experimental runtime-only optimization. **Not a stable release.**

## Source evidence

The user-supplied generated C++ in `cpp_ct2.zip`, `generated_program_part_27.cpp`,
defines `recomp_8C1560B0` with one block that performs
`dc_runtime_tick_fast(ctx, runtime, 3u)` followed by `ctx.pc = ctx.pr`.
The active callback at `0x8C157878` loads the target from `0x0C300B38`
and calls it indirectly; the 2026-10-19 CT2 log reports target
`0x0C1560B0` as the dominant indirect call (1,124,769,709 events).

These observations **do not** establish that skipping a wait loop would
preserve emulated behavior; no fast-forwarding of that loop is attempted.

## Optimization

`DCR_SH4_RETFAST=1` (default **off**) asks the runtime to replace a cached
registered AOT callback with a smaller native leaf **only** when its live
main-RAM instruction pair is exactly SH-4 `RTS; NOP`, machine bytes
`0B 00 09 00`. The leaf preserves three SH-4 cycles, PR/PC, normal
scheduler invocation, and async redirects. The existing native callback
is retained as a fallback and restored if the live bytes change.

All cached indirect calls continue using the existing dispatch cache.
No generated game DLL, ABI1 header, IPC, CD image or game source is modified.
The path is deliberately runtime-only so the existing compiled game cache
can be used for an A/B benchmark.

If the game binary uses different instruction bytes or the first dynamic
cache entry has already been populated, the optimization will not install.
Look for `[DCR SH4 RETFAST] installed` in the enabled-run log.

## Test

1. Extract the complete experimental portable ZIP.
2. Start `scripts/perf/TEST_CT2_RETFAST_AB.bat "C:\Games\Crazy Taxi 2.cdi"`.
3. Part A runs unchanged with `DCR_SH4_RETFAST=0`. Let the **same**
   game scene run for roughly 60 seconds before closing it.
4. Part B automatically starts with `DCR_SH4_RETFAST=1`; repeat exactly
   the same scene and duration.
5. Upload **both** timestamped session logs, identify any audio/display
   regressions. Compare FPS after boot/warm-up, not the splash screen.

Disable the CT2 callback probe and full SH-4 tracing for FPS comparisons;
those diagnostics add overhead. The only behavioral difference under
test should be the new RTS/NOP dispatch implementation.

## Automated tests

`dcr_sh4_trivial_ret_off` and `dcr_sh4_trivial_ret_on` validate:
return PC, three-cycle accounting, opt-in cache replacement,
and invalidation/fallback when the code bytes are overwritten.

**There is no measured FPS improvement until the user tests the game.**
