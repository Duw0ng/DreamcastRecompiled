# Validation — cpp34d IPBIN_REENTRY_SHADOW_GUARD_ABI1

Validation performed in the build environment before packaging.

## Source consistency

`python3 VERIFY_CPP34D.py` passes all required build markers.

## C++ runtime build

Configured and built `runtime/` in Release mode with C++20.

Results:

- `DreamcastRuntime`: PASS
- `dcr_network`: PASS

The compiler emitted an existing optimization warning in the PPP vector path; it did not fail the build and is outside the cpp34d IP.BIN changes.

## IP.BIN re-entry synthetic test

Sequence tested:

1. Fill `0x8C008000-0x8C00FFFF` with a known bootstrap pattern.
2. Call `dc_setup_commercial_boot` and capture the immutable shadow.
3. Execute a game-region PC (`0x8C010100`) to arm re-entry detection.
4. Overwrite all 32 KiB of IP.BIN with zeroes.
5. Seed an old IP.BIN relocation alias and dispatch-cache entry.
6. Re-enter at `0x8C008400`.

Result:

`SELFTEST bytes=1 aliases=1 cache=1`

Observed diagnostic:

`[DCR IPBIN] reentry detected | reason=tick | pc=0x8C008400 | restored=32768 | ... | dispatch-cache=flushed | count=1`

## Odd SH-4 target guard

Target `0x8C00F9A3` was injected from guest PC `0x8C00D994`.

Result: PASS. The runtime rejects it before treating it as an AOT missing target and reports:

`target-align=odd-invalid-sh4`

## Real-game validation still required

This environment does not execute the Windows/D3D11 ChuChu Rocket CDI runner. Use `TEST_CPP34D_IPBIN.bat` on Windows with `ccr.cdi`, then inspect the new session log for the cpp34d build markers and IP.BIN re-entry event.
