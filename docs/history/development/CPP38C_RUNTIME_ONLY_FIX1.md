# cpp38c runtime-only FIX1

Packaging fix only; SPG runtime code is unchanged.

- Removed invalid 16-byte `tool-placeholder` files named `dc_disc_probe.exe` and `dc_boot_prepare.exe`.
- `run_game.bat` now verifies that cached helper `.exe` files begin with the Windows PE `MZ` signature.
- Invalid/missing helpers are rebuilt as the two explicit CMake targets `dc_disc_probe` and `dc_boot_prepare`.
- With `--reuse`, `dc_raw_recomp` is not built and the guest SH-4 source/module is not configured, patched, rebuilt, or relinked.
- Runtime/plugin rebuild remains separate and may occur once for the cpp38c SPG change.
