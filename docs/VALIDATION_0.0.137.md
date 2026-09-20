# DreamcastRecomp 0.0.140 validation

Status: **experimental**. Stable rollback remains **0.0.131**.

## Change under test

0.0.140 replaces 0.0.136's fixed local PREF lookback with exact live Store Queue producer provenance. Local and CFG recognition share the same transfer rules; cross-block joins require MUST provenance from every reachable predecessor. The runtime TA hit guard is reduced to the mathematically equivalent Store Queue/QACR/FIFO bounds test and retains exact `dc_pref()` fallback.

## Validation

- Release core build: PASS, strict `--parallel 1`.
- CTest: **50/50 PASS**.
- Exact local Store -> PREF emission: PASS.
- Same-block definite overwrite kill: PASS.
- Cross-block MOV alias propagation: PASS.
- Cross-block definite overwrite kill: PASS.
- CFG MUST-join rejection when only one predecessor carries provenance: PASS.
- Native TA guard boundary/random equivalence: PASS (boundary matrix + 1,000,000 deterministic samples).
- Indexed PVR equivalence test: PASS.
- Specialized TA decoder test: PASS.
- Stable marker: `CURRENT_STABLE.txt = DreamcastRecomp 0.0.131`.

## Emulator architecture review

Flycast remains the most useful open modern reference. Relevant patterns reviewed for future work:

- TA state machine common-path dispatch with branch-likelihood bias and packet-loop unrolling.
- Polygon header selects one specialized vertex handler, then consecutive vertices use it directly.
- Continuous vertex storage plus index/primitive-restart representation.
- Texture cache invalidation driven by VRAM dirty/protection state and palette hashes rather than unconditional rebuilds.
- Optional threaded renderer queue that separates renderer Process/Render from SH-4 execution while preserving render completion synchronization.

Only the safe producer-dataflow/common-path guard work is included in 0.0.140. Packet batching, texture invalidation changes and render-thread separation remain future isolated experiments so their effects can be measured independently.
