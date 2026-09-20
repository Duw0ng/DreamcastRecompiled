# FPS audit — DreamcastRecomp 0.0.155

## Goal

The current priority is sustained 60 FPS in ChuChu Rocket!, including Mouse Mania. Compatibility work remains important, but changes below are ordered by expected CPU-time impact in the supplied stress traces.

## What 0.0.155 removes from the hot path

- 0.0.154 dynamic FR/XF valid/dirty masks and lazy-load branches.
- Per-instruction FTRV/FIPR/FMAC/FSRRA/FSCA global telemetry writes in cached FPU regions.
- Per-transfer generated `sq_writes` and FMOV64 telemetry-only writes.
- Normal native-TA PREF bookkeeping (`pref*`, `sq_commits`, `last_pref*`) unless explicit perf/provenance diagnostics are enabled.
- Per-packet TA capture-count/peak updates; these are accumulated once at parse flush.
- Per-vertex Type-7/8 packet/source/last-PC accounting; these are accumulated once per typed run.

None of those counters participate in guest-visible state or rendering.

## FPU: current direction

0.0.155 keeps two strategies in the same generated executable:

- `region`: the proven 0.0.153 basic-region cache.
- `superblock`: compile-time FR/XF set, no per-lane dynamic masks, persistence across safe internal CFG edges.

Flycast audit confirms that changing arithmetic precision is not a safe FPS shortcut:

- FIPR accumulates in double precision before the final float result.
- FTRV accumulates each dot product in double precision before the final float result.
- FMAC requires fused multiply-add semantics in the reference interpreter.
- x64 FSRRA is emitted as scalar square-root followed by scalar division, not approximate reciprocal sqrt.

DreamcastRecomp therefore keeps those numerical shapes and attacks surrounding register/memory traffic instead.

## Store Queue / TA producer: highest next candidate

The supplied 0.0.154 full-session log reached roughly 685 million SQ write accounting units and about 68.7 million TA PREF commits. 0.0.155 removes the diagnostic writes attached to those operations, but the guest still performs the architectural sequence:

1. write 32 bytes into SQ0/SQ1,
2. calculate/validate the PREF target,
3. copy the same 32 bytes into TA staging,
4. later parse the staged packet.

The next high-value AOT optimization is **producer fusion**: recognize a proven four-store / 32-byte SQ producer ending in a TA PREF and generate one guarded packet construction/submit path. The fallback must remain the existing architectural Store Queue path. This can remove repeated address tests, intermediate SQ memory traffic, and reload of the same packet at PREF.

This should be attempted before general graphical-fidelity features because the stress trace identifies CPU TA/SQ traffic, not D3D11 draw time, as the dominant scaling cost.

## Other FPS candidates, ordered

1. **TA SQ producer fusion** — expected highest remaining PVR-side CPU opportunity.
2. **Superblock CFG refinement** — if whole-function static sets cause register pressure, form static superblocks from hot strongly-connected CFG regions rather than complete functions.
3. **Host x64 vector backend for FTRV/FIPR** — only if generated MSVC assembly shows redundant loads/conversions. Preserve double accumulation/order; do not switch to float or approximate math.
4. **Eliminate remaining diagnostic writes in normal mode** — audit `ta_packets`, source histograms, strip counters and dispatch counters after 0.0.155 measurements.
5. **TA staging copy reduction** — raw payload is already 32 B + 1 B source. Further gain requires avoiding a copy, not shrinking metadata further.
6. **Compiler specialization** — evaluate AVX2/host-specific code generation and more aggressive inlining as an optional build target after correctness/performance A/B. Do not silently make the package incompatible with older x64 CPUs.
7. **Texture/RTT work** — lower priority for ChuChu FPS because previous logs show very high texture reuse and no meaningful readback pressure.
8. **Modifier volumes/fog/bump/mipmaps** — compatibility/fidelity priorities, not expected to recover the current Mouse Mania CPU deficit.

## Measurement rule for 0.0.155

Compile the commercial game once with the normal runner. Then use the two no-recompile BATs on the same executable:

- `run_commercial_recompiled_fpu_region.bat`
- `run_commercial_recompiled_fpu_superblock.bat`

Prefer one Mouse Mania in each run. Compare `fps`, `fpu-mode`, `fpu-cache`, `fpu-super`, `pvr-ta-stage-ms`, `pvr-cpu-est-ms`, `pvr-gcpu-ms`, `pvr-stripgpu`, `pvr-tametric`, `pvr-ta-pack` and `pvr-prov`.

Because normal 0.0.155 intentionally uses lightweight hot diagnostics, `sq-writes`, `fmov64` and native-PREF counters are no longer exact measures of total traffic. Use `--perf-profile` or `DCR_PVR_PROVENANCE=1` only when detailed diagnostics are needed; those modes are not valid for absolute FPS comparison with the normal lightweight path.
