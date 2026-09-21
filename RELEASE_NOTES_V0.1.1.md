# DreamcastRecomp v0.1.1 Official

## Focus

v0.1.1 is a compatibility-recovery release centered on **Crazy Taxi 2**. It keeps the modern v0.1 runtime/controller work while restoring the commercial-boot and SH-4 closure pieces that were present in the historical CT2 gameplay branch but were lost during universal-runner consolidation.

## Crazy Taxi 2 fixes

- CT2 now starts through the normal commercial bootstrap instead of the later direct-game bypass.
- `--ct2-compat` restores the Katana-compatible 32-byte scrambled executable staging expected by the IP.BIN bootstrap, preventing the double-descramble corruption that produced the invalid `0x005DFFFF` target.
- CT2-compatible generated code re-reads mutable PC-relative literals from guest RAM so bootstrap/self-modifying state is not replaced by a stale baked literal.
- The raw analyzer recovers ordinary GPR ABI functions immediately following an `RTS` delay slot; `0x8C16BFB6` is the regression case that exposed the gap.
- The CT2 profile contains the runtime-proven supplemental roots accumulated through the historical gameplay branch, including the post-title `0x8C07D9AA` family and 43 audited supplemental roots from the 0.0.203 package. These are addresses only; the recompiler regenerates code from the user's own disc image.
- `--ct2-compat` also enables a narrow Shinobi allocator-state recovery needed when `syMallocInit` was skipped by the commercial HLE path.
- The normal controller path remains keyboard + XInput + PS4/DirectInput, including optional `profiles/controller_profile.ini`.

## Validation

A clean v0.1.1 regeneration from the user's CDI produced **4,043 registered SH-4 functions**, **351,385 known SH-4 instructions**, **0 unknown SH-4** and **RAW_SH4=0**. The resulting runner was compiled and executed through the commercial boot route.

The automated regression path reached real 3D gameplay and rendered the taxi, HUD, traffic and city scene. The run continued to the controlled **10,000,000 PVR-packet probe limit**; the last full heartbeat before that stop recorded **9,959,626 PVR packets, 2,670 frames and 300 GD-ROM requests**. There were no missing native targets or SH-4 faults; the only final error was the intentional diagnostic packet-limit stop.

The public package does **not** contain a game image or pre-generated Crazy Taxi 2 game code. `run_game.bat` analyzes the user's CDI and regenerates the native output locally.

## Usage

```bat
run_game.bat "C:\Games\Crazy Taxi 2.cdi"
```

For normal play no diagnostic flags are required. `--ct2-compat` is selected automatically by the Crazy Taxi 2 profile.
