# Validation — DreamcastRecomp 0.0.40.1

## Purpose

0.0.40.1 is a Windows-focused maintenance release for the reported `sound/sfx` live behavior: the runner prints `Executing _main` but gives no visible window, no audible response and no useful progress signal.

## New runtime diagnostics

Generated runners accept:

```text
--host-window
--diag-heartbeat-ms=1000
```

The host window is not a PVR window and does not pace the guest. It exists only to prove that the native runner process is active. A heartbeat line includes:

```text
guest-pc
arm7-pc
arm7-reset
aica-frames
native-starts
maple-status
maple-host-polls
host-syncs
```

This makes the next Windows report actionable. In particular:

- `aica-frames=0` with `arm7-reset=yes` means bootstrap has not released ARM7.
- growing `aica-frames` with `native-starts=0` means the mixer/device clock is alive but no AICA slot was keyed on.
- growing `native-starts`/`aica-frames` with silence points to WinMM/output or sample/mixer state rather than SH-4 bootstrap.
- `maple-status=0` means the application has not reached its interactive controller loop yet.
- growing `maple-status`/`maple-host-polls` with no button response isolates the problem to host input mapping/state.

## Windows commands

Interactive live test:

```bat
run_homebrew_sfx_live.bat "C:\path\to\sfx.elf"
```

Automatic comparison test:

```bat
run_homebrew_sfx_autotest.bat "C:\path\to\sfx.elf"
```

The live test also requests a WAV at:

```text
generated\sfx_live\sfx_live_capture.wav
```

If the interactive run still appears stuck, the most useful evidence is 5-10 consecutive heartbeat lines plus whether the automatic test produced/heard audio.

## Local regression

```text
CTest: 47 / 47 PASS
```

A generated standalone sample project also builds on Linux and runs its generated compile test. The actual Win32 status window and WinMM device remain Windows-only validation items.
