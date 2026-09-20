# DreamcastRecomp 0.0.134 validation

Stable rollback point: **0.0.131**.
Development parent: **0.0.132**.

## Functional validation

- CTest: 48/48 PASS (47 existing + `pvr_indexed_tests`).
- Fresh emitter/runtime test: PASS.
- Fresh generated C++ configure/build: PASS.
- `generated_compile_test`: RC=0.
- generated literal-pool runner: RC=0 / expected output.
- Indexed synthetic triangle equivalence: PASS.
- Generated runtime contains `IASetIndexBuffer(...DXGI_FORMAT_R32_UINT...)` + `DrawIndexed`.
- Windows build policy remains serial; `/MP` is not emitted.

## Synthetic old-vs-indexed comparison

Workload: 1000 strips x 20 vertices = 20,000 unique TA vertices and 18,000 triangles (54,000 triangle vertex references).

Observed on the validation host (Release, median of 101 repetitions):

- old deferred storage: 2,016,000 bytes
- indexed deferred storage: 1,216,000 bytes (~39.7% lower)
- old logical GPU vertex upload: 1,512,000 bytes
- indexed VB+IB upload: 776,000 bytes (~48.7% lower)
- old registration: ~0.85 ms
- indexed registration: ~0.20 ms
- old BUILD conversion: ~0.065 ms
- indexed BUILD conversion+indices: ~0.043 ms

Absolute timing is host-specific and is not an FPS forecast. Live Windows validation should compare heartbeat deltas for `pvr-cpu-est-ms`, `pvr-gcpu-ms`, FPS and the new `pvr-idx` ratio during the same Mouse Mania segment.
