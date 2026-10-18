# DreamcastRecompiled V1.2 Stable Release — DCRV1.2

Baseline: user-validated AQUA RC8, including the precompiled ChuChu Rocket! portable.

- The public branding, window titles, and packaging use the DCRV1.2 identity.
- `games/CHUCHU_ROCKET/` retains its original executables, DLLs, and SHA-256 checksums.
- `safe_guests/` remains the unmodified fallback.
- The validated precompiled path does not recompile SH-4 on each launch.
- The independent VMU provides a selectable Windows taskbar icon (Win32 API), an always-on-top setting enabled by default, and an **Open / Show VMU** button to recover its window.
- Borderless mode preserves the original window shell and VMU preferences.

## Technical identifiers

`cpp38e`, `cpp35b1`, `DCR_CPP38E_*`, `RUNTIME=...`, `DCR_GUEST_IMPL_REV`, and internal functions and paths **were not renamed**. These values are part of build identities, hashes, and cache compatibility. Altering them might force recompilation or break ABI1. Historical documentation retains its original development filenames for traceability.

## Original validation scope

The ZIP was assembled by reusing the exact offline Python host, runtime, and RC8 ChuChu portable binaries. Real Windows rendering, taskbar behavior, and gameplay required separate verification at the time of this original report.
