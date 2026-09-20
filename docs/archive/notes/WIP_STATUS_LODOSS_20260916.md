# DreamcastRecomp 0.0.204 — Lodoss WIP snapshot (2026-09-16)

This ZIP is a checkpoint of the current DreamcastRecomp work before continuing the Record of Lodoss War bring-up.

## Core source changes included

- Generic recognition of runtime-copied/relocated SH-4 helpers beyond the previous VBR-only pattern.
- Better relocated-code dispatch using a proven relocation delta to resolve ambiguous internal entrypoints.
- Runtime-aware PC-relative literal loads for self-modifying/bootstrap code.
- FPU FPSCR.RM support extended to accept SH-4 RM=01 (round toward zero); RM=10/11 remain reserved.
- PVR Channel-2 DMA routing distinguishes TA, texture and YUV apertures.
- PVR YUV420 macroblock conversion to YUV422/UYVY VRAM output.
- YUV422 texture sampling/conversion to RGB in the software PVR path.
- PVR YUV status/event handling used by the Lodoss/CRI video path.
- Broader callback discovery for dense pointer tables, with the current conservative minimum run lowered to 8 entries and decoder-clean confidence checks.
- Recognition of compact `RTS` leaf callbacks with `MOV #0,R0` delay slots.
- Generated runner now accepts `--r15=...` (R0-R15 instead of R0-R14), required to reproduce the selfboot handoff stack used in the Lodoss tests.

## Lodoss state at this checkpoint

- Main executable identified as `1NOSDC.BIN`, loaded at `0x8C010000`.
- 7,280-function base AOT closure generated with Unknown SH-4 = 0 and RAW_SH4 = 0.
- Correct selfboot handoff used for direct-main tests: `R15=0x8C00F400`.
- YUV video path works: first real visible frames were rendered successfully.
- START can be injected at Maple GETCOND level to skip the initial movie in the diagnostic runtime.
- The game advances past the initial movie and the long `Now Loading...` stage into later scene/title initialization.
- Latest unresolved dynamic entrypoint before this snapshot: `0x8C090380`.
- The experimental incremental compatibility layer contains all discovered entrypoints up to (but not including) that next target.

## experimental_lodoss_wip

This directory preserves the exact WIP materials used during the bring-up:

- `lodoss_known_extras.cpp`: accumulated incremental SH-4 functions/targets not present in the 7,280-function base closure.
- `lodoss_registered_targets.txt`: unique runtime targets registered by that incremental module.
- `function_map.csv`: 7,280-function base function map.
- `sh4_address_taken_evidence.csv`: address-taken/callback evidence from the base closure.
- `generated_runner_known.cpp`: diagnostic runner variant that registers the incremental module and accepts R15.
- `dc_runtime_autostart.cpp`: diagnostic runtime variant used for low-level START injection during Lodoss testing.
- `append_lodoss_seed.py`, `build_lodoss_extras.py`, `lodoss_iter.sh`, `lodoss_add_only.sh`: exact session helper scripts. They contain `/mnt/data/...` paths from the test environment and are preserved for reference; adjust paths when using them elsewhere.

## Game data

No Record of Lodoss War CDI/RAR, `1NOSDC.BIN`, or other game data is included in this ZIP. Use your own image and the existing commercial-disc extraction flow.

## Verification at packaging time

- `dc_raw_recomp` rebuilt successfully after the final source changes.
- CTest `cpp_emitter_tests`: PASS.
- CTest `dc_recomp_codegen`: PASS.
