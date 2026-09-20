# DreamcastRecomp 0.0.209 — Universal Boot consolidation

## Goal
One source/runtime base for the current commercial test set: ChuChu Rocket!, Crazy Taxi 2, Daytona USA and Record of Lodoss War.

## Consolidated fixes
- Keeps the SH-4/FPU/PVR/AICA/G2/runtime work carried by the 0.0.204–0.0.208 branches instead of maintaining a separate per-game source fork.
- Keeps the generic ChuChu `0x8C02CE0C` recovery rule from the post-0.0.204 closure work.
- Keeps Lodoss runtime-relocated SH-4 dispatch, mutable PC-relative literals, YUV path and related PVR fixes from the 0.0.204 Lodoss WIP.
- Adds `dc_raw_recomp --seed-file=FILE`. Compatibility roots are data files, not hard-coded title checks in the recompiler.
- Adds `dc_disc_probe --extract-file=NAME=FILE` for titles such as Record of Lodoss War that chain to a secondary executable.
- Adds `run_universal_0.0.209.bat`, which inspects the CDI title and selects the correct preparation/profile while keeping one common source tree and generated runtime.

## Compatibility profiles
- `profiles/daytona_usa_known_seeds.txt`: the 54 proven Daytona FIX1 roots formerly embedded as a long BAT command line.
- `profiles/record_of_lodoss_war_known_seeds.txt`: function roots consolidated from the validated Lodoss WIP incremental closure, plus the latest observed unresolved target.

## Important design note
The profiles are an interim compatibility database. The preferred long-term direction remains promoting these roots through generic structural discovery. Moving them to data already removes the source-code/game-fork dependency and makes regressions reproducible from the same 0.0.209 base.
