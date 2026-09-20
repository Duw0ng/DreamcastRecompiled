# DreamcastRecomp 0.0.56 validation

- CMake/CTest Debug regression: **47/47 PASS**.
- Runtime font backing size: `0x00100000` bytes from physical base `0x00100000`.
- `0xA012BAC0` aliases physical `0x0012BAC0`, which is inside the mapped read-only synthetic region.
- Analyzer/recompiler closure logic is unchanged from 0.0.55; no new title seed or RAW_SH4 fallback was added.
- PVR hot path and timing are unchanged from 0.0.55.

- Direct generated-runtime probe at `guest_pc=0x8C0227D0`: reads at `0xA010A070`, `0xA012BAC0`, and `0xA01FFFFF` all PASS.
