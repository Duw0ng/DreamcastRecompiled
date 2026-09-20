# DreamcastRecomp 0.0.194 validation — rebased from 0.0.193

## Parent / compatibility policy
- Sole functional parent: 0.0.193. The previous 0.0.194/0.0.195 scheduler changes are not included.
- Commercial SH-4 tick batch remains 256 cycles. No new IRQ, GD-ROM, Holly, TMU, AICA or device-clock scheduling behavior is introduced.
- CT2 0.0.193 wait157 fast-forward is retained unchanged.

## Performance target from the supplied 0.0.193 profile
- The supplied CT2 session reaches 105,596,142 PVR packets over 11,612 PVR frames (~9,094 packets/frame).
- `pvr-ta-stage-ms=143797`, about 12.38 ms of staged TA parsing per frame over that run, makes TA/PVR CPU work the primary conservative optimization target.
- The 0.0.193 log shows the established 256-cycle SH-4 batch and therefore this build deliberately avoids changing batch size.

## Changes
- `TA_ISP_CURRENT` (PVR 0x005F8138) is maintained in a compact runtime field after `TA_LIST_INIT`; guest reads return the same value without a per-packet hash lookup/write.
- `pvr_ta_front_consume` is force-inlined. The state-neutral `pt=7 + PLV32` packet case returns before the command-only decode chain.
- A staging self-test checks exact `TA_ISP_CURRENT` initialization and +32-byte advancement.

## Validation
- Main CMake Release build: PASS.
- Core CTest suite: 51/51 PASS.
- Generated CT2 `dc_runtime.cpp` targeted C++20 `-O3` compilation: PASS.
- Full generated CT2 build reached the huge AOT shards but exceeded the environment command window; no compiler diagnostic was emitted before termination.
