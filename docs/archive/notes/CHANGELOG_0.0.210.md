# DreamcastRecomp 0.0.210 — Universal controller integration

## Base
0.0.210 is built directly on the consolidated 0.0.209 universal commercial tree. It retains the common SH-4/FPU/PVR/AICA/G2 fixes and compatibility seed profiles used by ChuChu Rocket!, Crazy Taxi 2, Daytona USA and Record of Lodoss War.

## New in 0.0.210
- Generated runners accept `--controller-profile=FILE`.
- `Configurar_Mando.bat` launches a Windows GUI for controller mapping and writes `profiles/controller_profile.ini`.
- Controller profile backends: `xinput`, `winmm` and `auto`.
- Device index and deadzone are configurable.
- Face buttons, Start, D-pad, C/Z, both analog sticks and both triggers can be mapped.
- The runtime supports WinMM joystick polling for devices exposed by Windows outside XInput.
- `run_universal_0.0.210.bat` automatically passes the saved controller profile when it exists.
- With no profile, the previous keyboard + XInput #0 mapping is preserved.

## Compatibility base retained
- Daytona USA: `profiles/daytona_usa_known_seeds.txt`.
- Record of Lodoss War: `profiles/record_of_lodoss_war_known_seeds.txt`.
- ChuChu Rocket!: generic commercial closure plus current SH-4/PVR/AICA fixes.
- Crazy Taxi 2: generic commercial closure plus current SH-4/FPU/runtime fixes.

## CT2 runtime validation added before final packaging
- The generic IP.BIN bootstrap path exposed a stale/dirtied literal at the CT2 hand-off (`0x005DFFFF`).
- CT2 now uses the already-supported direct commercial game entry at `0x8C010000`, preserving the commercial BIOS/HLE hooks and GD-ROM map while avoiding that invalid bootstrap hand-off.
- Added `profiles/crazy_taxi_2_known_seeds.txt` with two clean runtime-discovered SH-4 entries: `0x8C16BF2A` and `0x8C16BFD4`.
- Final CT2 static closure with those seeds: 3,823 functions, 331,876 reachable/known SH-4 instructions, 0 unknown SH-4, 0 RAW_SH4.
