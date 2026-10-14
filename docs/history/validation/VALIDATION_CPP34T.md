# Validation — cpp34t Late-AOT 0x8C036118

## Runtime/network preservation

cpp34t is based byte-for-byte on cpp34s for the execution path:

- `runtime/plugins/dcr_network.cpp` SHA-256: `f47a6a4da8cec3a8ad95e39556cb9d672ea3f40ba97c3517021adb8b07b991bc`
- `runtime/include/dc_runtime.hpp` SHA-256: `ca75e79316dc78ae74021a0853312f4c3f378a0f0524be0371c0c3db263e9739`
- `src/codegen/cpp_emitter.cpp` SHA-256: `e56050f0864d1c9272334d6e8a06214011383cbc641345f743286b7889444bf4`

Therefore cpp34s V.42/LAPM reliability, TCP fast path, ABI1, and its 54/54 core validation are unchanged.

## New runtime boundary

The cpp34s user log reached the online submit, advanced PPP/IP/TCP, received a server response and then stopped at:

`No recompiled/native target registered for Dreamcast address 0x8C036118`

Nearest registered Late-AOT target: `0x8C036102`, delta `0x16`.

The 16 target bytes reported by the runtime decode as 8/8 known SH-4 instructions with `RAW_SH4=0` in `dc_raw_recomp`. Actual PC-relative literal values are intentionally compiled from the user's full cached `BOOTSTRAP.BIN`, not from the diagnostic byte prefix.

## Dual-target overlay validation

`BuildLateTargetPatch.py` now accepts repeated `--target` arguments.

Synthetic ABI1 smoke:

- `--target 0x8C036102 --target 0x8C036118`
- Synthetic entries: 2
- Reachable functions: 2
- Known SH-4: 4
- Unknown SH-4: 0
- RAW_SH4: 0
- Registered functions: 2
- `dcr_guest_patch` linked successfully against DreamcastRuntime
- persistent seed file contains both targets
- main guest cache was not regenerated or rebuilt

## Windows path

Run:

```bat
TEST_CPP34T_LATE_AOT2_ONLINE.bat "C:\ruta\ccr.cdi"
```

If the existing seed file lacks either target, the test performs the small overlay migration automatically and then launches with `--reuse --rebuild-runtime`.
