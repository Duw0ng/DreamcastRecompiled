# DreamcastRecomp 0.0.167 validation

## Scope

0.0.167 is the Region+ recovery build derived from the live 0.0.165 `region`, 0.0.166 `blockssa166`, and 0.0.166 `superblock` Mouse Mania comparisons. No TA/PVR, scheduler, host-sync, AICA/CDDA, Present, GPR-cache, or texture behavior is intentionally changed from 0.0.165/0.0.163.

## Production FPU path

- default runtime mode: `regionplus167` (`DCR_FPU_MODE=regionplus`);
- restores the proven per-region `uint32_t` FR/XF locals, including cached XF0..XF15 for FTRV;
- keeps `dc_host_fma` and generated MSVC Release `/arch:AVX2` + FMA3;
- keeps FIPR/FTRV double accumulation and final float writeback;
- production Region+ sets `DCR_FPU_METRICS=0`, avoiding per-region global diagnostic counter writes;
- `run_commercial_recompiled_fpu_region.bat` enables the legacy region telemetry path without recompilation;
- `run_commercial_recompiled_fpu_superblock.bat` keeps the superblock A/B path;
- `run_commercial_recompiled_fpu_regionplus_metrics.bat` re-enables Region+ counters when diagnosis is needed.

## Conservative writeback cleanup

- `FMOV FRn,FRn` is treated as an identity and does not dirty the lane;
- when the final cached instruction is a control boundary, the pre-boundary synchronization is authoritative and the duplicate final writeback is omitted;
- no FR/XF value is kept dirty across scheduler/tick boundaries.

## Host validation

- Release source build: PASS.
- CTest: **50/50 PASS**.
- Fresh generated FPU arithmetic project: configure/build PASS; `generated_compile_test` exits 0.
- Fresh generated FPU unary/vector project containing FIPR/FTRV: configure/build PASS; `generated_compile_test` exits 0.
- Commercial ChuChu Rocket! is not available in the host validation environment; FPS results require the Windows live test.

## Expected live markers

Normal launcher:

- `build=0.0.167`
- `fpu-mode=regionplus167`
- `fpu-metrics=off`
- `host-fma=fma3` on the default MSVC AVX2 build
- `gpr-cache=0/...`
- `host-sync-q=262144`

Primary comparison: FPS versus `pvr-packets/frame` at 15k, 18k, 20k, and 22k+ packets/frame against the 0.0.165 region log. The intended win is to preserve region's heavy-load slope while reducing its fixed telemetry overhead.
