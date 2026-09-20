# DreamcastRecomp 0.0.120 — validation

## Goal

Reduce host overhead in the general SH-4 dynamic-dispatch hot path without changing target resolution or guest-visible semantics.

## Live input motivating this version

0.0.119 reached ~70 FPS peak and ~60–63 FPS normal gameplay. In the supplied heavy-rat recovery segment FPS progresses 41, 43, 45, 46, 53, 54, 52. `dispatch-hint` is ~95–97% accurate where present, but covers only a few thousand calls/frame while `dispatch-inline` remains above ~100k/frame.

## Change

`fast_dynamic_dispatch_ready` and `direct_dispatch_ready` cache the existing dispatch-safety predicate. They are refreshed at runner configuration and relocation-context transitions. Generated dynamic/static/direct call paths use the latch instead of re-reading every invariant gate on every call.

## Acceptance

- Core CTest suite must remain 47/47 PASS.
- Fresh generated compile/run smoke test must succeed.
- Commercial heartbeat should report `dispatch-ready=1/1/...` during ordinary gameplay.
- PVR, audio/CDDA, input and guest timing are unchanged.

## Local result

- CTest: 47/47 PASS.
- Fresh generated host build: PASS.
- `generated_compile_test`: exit 0.
