# DreamcastRecomp v1.2 cpp38e — HOT BLOCK RECOVERY

- Targets host-side cost identified by cpp38d at ChuChu PCs `0x8C0FE5A0` / `0x8C1088DA`.
- HOTPC profiler remains available but is disabled for normal cpp38e benchmarking.
- One-time reuse of the existing generated guest source; no SH-4 rediscovery is required.
- Main-RAM 32-bit accesses use the validated MMU-off guest fast path for ChuChu.
- Hot tick, dispatch, direct-hint and current-PC diagnostic stores are reduced/removed from the optimized guest.
- `dynamic_direct_hint_hits` now respects lightweight hot metrics for newly generated source and is patched in cached generated source before rebuilding.
- ABI v1 remains unchanged; no guest-code skip, clock cheat, or new timing fast-forward is introduced.
