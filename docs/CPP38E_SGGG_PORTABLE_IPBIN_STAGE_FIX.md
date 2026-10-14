# cpp38e: portable combined retail staging fix (SGGG early-boot regression)

Evidence from `DreamcastRecomp_v0.1.1_session_20261011-042743.log`:
- Source staged from ENV at `0x8C008000`, **3,138,871 bytes**.
- Retail SGGG combined BOOTSTRAP.BIN contains **32,768-byte IP.BIN** + **3,106,103-byte main executable**.
- Old log showed `[DCR BOOT] staged scrambled commercial executable @0x8C008000 (3138871 bytes)`.
- Bootstrap then misdirected a SH-4 call at `0x8C00D9A0` into text bytes at `0x8C00E2A2` (`@nBug hunting, b`), and failed before any GD-ROM request or PVR frame.

## Root cause (confirmed in source)

`dc_setup_commercial_boot` previously loaded the combined image from portable `DCR_EXTERNAL_RAW_IMAGE`, then overwrote its retail staging metadata with the **full combined image size and initial IP.BIN address**. As a result, the `scrambled` mode shuffled *IP.BIN and the executable together*, even though the generated runner already had split metadata for precisely this situation.

## Correction

- Preserve every byte of the 32 KiB IP.BIN at `0x8C008000`.
- When portable raw loading starts at `0x8C008000` with more than 32 KiB, use `0x8C010000` and `raw_size - 0x8000` as the scrambling region.
- Keep the existing behavior when loading an executable-only image directly at `0x8C010000`.
- Keep non-scrambled (`plain`) mode byte-for-byte unchanged.
- Bump runtime build ID to force separate runtime DLL rebuild while preserving existing compiled SH-4 guest/module caches.
- No PVR/AICA/Maple/network/guest ABI code changes.

New log expected for SGGG:
```
[DCR PORTABLE] retail IP.BIN preserved (32768 bytes) | executable-stage=0x8C010000 | executable-bytes=3106103
[DCR BOOT] staged scrambled commercial executable @0x8C010000 (3106103 bytes)
```

## Validation

- Linux native `DreamcastRuntime` build: PASS.
- Linked integration regression `dcr_portable_ipbin_stage_tests`: 4/4 cases PASS: combined scrambled, combined plain, executable-only scrambled, executable-only plain.
- Real SGGG CDI boot on Windows/MSVC: **not yet tested**; user validation still required.
- Keep cpp38e as official stable until title boot and New Game validation.

## On Windows

After extracting the full ZIP, run:
```
run_game.bat "C:\\ruta\\SGGG.cdi" --reuse --debug
```
The launcher should rebuild the *runtime DLLs only*, without SH-4 recompilation. If a previously pending per-game limits change blocks reuse, complete that one explicit per-game rebuild first:
```
run_game.bat "C:\\ruta\\SGGG.cdi" --recompile-game --debug
```
Send the newest session log if boot does not reach the original cpp17/18 intro/menu.
