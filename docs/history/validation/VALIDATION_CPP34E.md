# Validation cpp34e BOOTSAFE_IPBIN_REENTRY

- Source base: cpp34d package created from cpp34c FLASH_BASE0_FIX_ABI1.
- Regression reproduced from user log: cpp34d restored IP.BIN at first dispatch to 0x8C00FA00 while PVR frames/flips were still 0.
- Root cause: `seen_game` was armed by any transient execution >= 0x8C010000 during normal bootstrap.
- Fix: restoration is now gated by `armed`, which requires `seen_game` plus at least one real PVR page flip.
- Early 0x8C00FA00 / 0x8C00F400 helpers therefore remain available for normal code relocation.
- Odd-target SH-4 guard retained.
- Full 32 KiB IP.BIN shadow retained.
- Dynamic alias/cache invalidation retained only for a post-arm reentry.
- Linux CMake validation: DreamcastRuntime built successfully; dcr_network built successfully.
- Existing GCC warning in dcr_network PPP vector code remains unchanged from prior implementation; no compile error.

Expected cpp34e telemetry:

- Boot: `ipbin=1/.../0/.../0` and NO `reentry detected` at 0x8C00FA00.
- Normal bootstrap should again emit `[DCR code-reloc] 0x8C00FA00 -> template ...`.
- After first visible frame: `[DCR IPBIN] reentry guard armed ...`.
- Later Homepage/DreamKey return: `[DCR IPBIN] reentry detected ... restored=32768`.
