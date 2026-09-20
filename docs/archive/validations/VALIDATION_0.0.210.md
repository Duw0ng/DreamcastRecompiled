# DreamcastRecomp 0.0.210 — final validation

## Core build
- Linux CMake/GCC source build: PASS.
- Core/generated test suite: 53/53 PASS.
- Generated single-program compile/link test: PASS.
- Generated multi-shard program compile/link test: PASS.

## Crazy Taxi 2 supplied-image validation
The user's CT2 CDI was used only as local validation input and is **not** redistributed.

- Disc probe identifies `CRAZY TAXI 2` and extracts the retail executable.
- Final commercial closure after the runtime-discovered CT2 seed profile:
  - closure passes: 12
  - reachable functions: 3,823
  - reachable instructions: 331,876
  - known SH-4: 331,876
  - unknown SH-4: 0
  - RAW_SH4: 0
- A full generated CT2 Debug/O0 runner with the first runtime seed compiled and linked successfully.
- Direct-game runtime smoke performs real GD-ROM requests/sector reads and exposed two additional clean SH-4 entry points (`0x8C16BF2A`, `0x8C16BFD4`), now retained in `profiles/crazy_taxi_2_known_seeds.txt`.
- The original generic IP.BIN-to-game hand-off is not used for CT2 in 0.0.210 because that path dirtied the first game-code literal page and produced an invalid `0x005DFFFF` dynamic target in the Linux smoke. The CT2 profile uses `--direct-game-entry=0x8C010000` instead.

## Controller integration
- Generated runner accepts `--controller-profile=FILE`.
- Universal Windows runner auto-loads `profiles/controller_profile.ini` when present.
- No profile keeps the legacy keyboard + XInput #0 behavior.
- The Windows GUI can write profiles for `auto`, `xinput`, and experimental `winmm` backends.

## Host limitation
Actual Windows window creation and physical XInput/WinMM device polling cannot be exercised in the Linux validation container. Those paths are guarded as Windows host code and the generated projects compile/link in the available environment.
