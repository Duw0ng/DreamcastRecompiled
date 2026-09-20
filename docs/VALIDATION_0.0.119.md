# DreamcastRecomp 0.0.119 — validation

## Goal

Use the 0.0.118 live profile to reduce SH-4 dynamic-dispatch host overhead without changing any guest-visible target selection or the PVR/audio paths.

## Live evidence from 0.0.118

During the heavy-rat interval represented by heartbeats #45 -> #46, the run produced about 40 frames while adding ~787k PVR packets, ~482k triangles and ~4.30M dispatch-cache hits. That is approximately 19.7k packets, 12.1k triangles and 107.5k dynamic dispatches per frame. Sampled PVR timing was about 6.1 ms/frame TA, 0.48 ms STATE, 2.5 ms GEOM and 1.95 ms GPU.

## Change under test

- Build up to four analyzer-proven direct candidates per dynamic CALL site from resolved indirect calls, branch-selected callback alternatives and known dynamic target sets.
- Compare the actual runtime target before selecting a direct candidate.
- On a match, call the owning generated C++ function directly and preserve exact trace history/PR/PC/async redirect behavior.
- On mismatch or when any relocation/trace/PVR-observer safety gate is active, use the existing dynamic dispatcher unchanged.
- Heartbeat adds `dispatch-hint=HITS/FALLBACKS`.

## Local regression

- Core CTest: **47/47 PASS**.
- Fresh generated dynamic-call sample: CMake Release build PASS.
- `generated_compile_test`: exit 0.
- Generated sample runner: exit 0, R0=42.

## Windows live acceptance

Use the same ChuChu Rocket heavy-rat event. Compare normal/minimum FPS against 0.0.118 and capture `dispatch-hint`, `dispatch-inline`, `dispatch-cache`, `sh4tick`, `pvr-cpu-est-ms`, `pvr-gpu`, `audio-starves`. A high `dispatch-hint` hit ratio should correlate with reduced CPU overhead; a low ratio is still actionable because it shows that the hot calls are more polymorphic than the analyzer candidates.
