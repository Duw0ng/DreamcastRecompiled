# DreamcastRecomp 0.0.164 validation

## Parent and scope

- Parent: live-proven 0.0.163.
- Host/AICA sync cadence remains 262,144 SH-4 cycles.
- Post-Present host sync remains enabled.
- Production function-wide GPR cache remains disabled.
- PVR deadline/UI guest-cycle gates remain unchanged.
- No texture, blend, depth, cull, translucent-order or audio semantic changes.

## Flycast-derived TA structure

The audit of current Flycast (`core/hw/pvr/ta.cpp`, `ta_vtx.cpp`, `ta_ctx.cpp`) found three useful patterns:

1. TA ingress is organized around compact 32-byte packet handling and small front-state updates.
2. Object Control selects concrete vertex handlers; decoded vertices are appended contiguously and strip finalization records a `first/count` range.
3. TA contexts are reset/recycled rather than treated as one-shot allocations.

DreamcastRecomp already had staged 32-byte packets, typed Type-7/8 decoding and persistent vectors. 0.0.164 closes the remaining hot-path gap by decoding a complete Type-7/8 strip as one unit instead of invoking generic strip bookkeeping per vertex.

## New direct strip path

Eligibility requires a complete staged Type-7/8 strip, normal replay, PERF profile off, an empty current strip, at least three vertices and the existing deferred-triangle path.

For an eligible strip the runtime:

- grows the vertex arena once;
- caches face base/offset color and probe mode once;
- decodes all positions/UV/intensity values in one typed loop;
- applies the same plausibility/finite-coordinate rules;
- performs the existing strip finalization exactly once.

If any direct-path guard fails, the vertex arena is rolled back and the exact pre-0.0.164 path handles the strip.

## Persistent high-water capacity

- Vertex arena warm reserve: 32,768 vertices.
- Opaque R32 index warm reserve: 65,536 indices.
- Opaque state-run warm reserve: 4,096 runs.
- Frame clears retain vector capacity.

## Telemetry

`pvr-flystrip=<runs>/<vertices>/<fallbacks>` reports successful complete strips, directly decoded vertices and direct-path fallbacks. The TA marker includes `flystrip164`.

## Automated validation

- Release host build: PASS.
- CTest: 50/50 PASS.
- Complete Type-8 generated self-test requires direct-strip coverage: PASS.
- Incomplete Type-8 generated self-test requires zero direct-strip coverage: PASS.
- Generated C++ project configure/build: PASS.
- `generated_compile_test`: PASS.
- Generated native hello runner: PASS; returns with PC=0xFFFFFFFF.

Commercial ChuChu Rocket! was not executed in this environment. FPS/coverage validation must therefore come from the user's Windows live log.
