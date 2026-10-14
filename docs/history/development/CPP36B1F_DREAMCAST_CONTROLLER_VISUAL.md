# cpp36b1f — Dreamcast Controller Visual

Launcher-only usability release based on cpp36b1e.

## Goal
The modern Maple controller setup now includes a full live Dreamcast controller view. The preview renders the state **after host input is mapped to Dreamcast**, so the user sees what the guest actually receives.

## Visualized controls
- Dreamcast D-pad
- A / B / X / Y
- START
- analog stick position (-128..127)
- L / R analog trigger fill (0..255)
- optional Maple C / Z bits used by some host shoulder mappings
- selected Dreamcast port A/B/C/D
- connected/disconnected state and active host backend

## Device independence
The existing per-port device selector remains unchanged. Each Dreamcast port can independently use Automatic, Disconnected, Keyboard + Mouse, XInput, or DirectInput/WinMM.

## No guest/runtime rebuild
This release only changes `portable/launcher/main_windows.go` and the compiled `portable/DreamcastRecompiled.exe`. `DCR_RUNTIME_BUILD_ID` intentionally remains `cpp36b1e-runtime-external-raw-env`, so `--reuse` does not rebuild Runtime/Maple solely for this UI update. The SAFE Guest ABI1 module is unchanged.
