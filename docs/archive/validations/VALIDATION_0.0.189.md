# DreamcastRecomp 0.0.191 validation

## Root cause targeted
The 0.0.188 CT2 trace showed the first persistent 12-VBlank stall at the same point where guest `PVR_ISP_START` writes stopped mapping 1:1 to runtime render starts/completions. Before the transition, `isp-start`, `pvr-frames`, and scheduled render-done counts tracked together. At the transition, `isp-start` advanced while `pvr-frames` / `pvr-rdone` did not, leaving CT2's active queue slot in state 4 until its ~10-VBlank recovery timeout.

The old runtime only called the hardware-visible render completion path when the host TA parser had produced `pvr_render_pending`. That incorrectly coupled Dreamcast STARTRENDER completion to host-side geometry recognition.

Flycast schedules render completion before checking whether a TA context exists. With no TA context it schedules render-done after 4096 SH-4 cycles; with a context it uses `min(450000 + size*100, 1500000)` cycles.

## 0.0.191 change
- Guest STARTRENDER is now accepted even when the host TA parser has no rasterizable buffer.
- No-context STARTRENDER schedules Holly render-done after 4096 SH-4 cycles.
- Context-backed STARTRENDER retains the existing Flycast-style size-based delay.
- Empty/no-context completion raises the three normal render-done bits but does not mark a stale host render buffer for page flip.
- Heuristic host-only render starts remain buffer-bound.
- No DCRuntime layout change was required; the no-context state uses an internal sentinel in the existing render-done metadata.

## Validation performed
- Core build: PASS.
- CTest: 51/51 PASS.
- `cpp_emitter_tests`: includes assertions for guest STARTRENDER decoupling, 4096-cycle no-context completion, and stale-buffer protection.
- CT2 `dc_runtime.cpp`: standalone C++20 compile PASS.
- CT2 `generated_runner.cpp`: standalone C++20 compile PASS.
- Generated SH-4 shard ABI is unchanged from 0.0.188 (no DCRuntime layout changes).

## Runtime check requested
During the previously slow CT2 3D section, compare:
- `isp-start=`
- `pvr-frames=`
- `pvr-rdone=sched/SCHEDULED/FIRED/...`
- `wait157-lat=.../LAST_VBLANK_DELTA`
- `ct2-slots=` / `ct2-idx=`

The key expected change is that a guest STARTRENDER no longer disappears just because the host parser has no frame geometry. If the missing completion was the cause, the state-4 slot should advance without waiting for `/12/12`, and FPS should recover accordingly.
