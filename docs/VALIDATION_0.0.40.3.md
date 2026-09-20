# Validation 0.0.40.3

## Automated suite

`ctest --test-dir build-linux --output-on-failure`: 47/47 passed.

## KallistiOS SFX host-clock performance probe

Input: supplied `dreamcast/sound/sfx/sfx.elf`.

Command profile: `_main`, KOS AICA defaults, real ARM7 firmware, host-synchronized
device clock, host Maple input, one second AICA capture.

Observed:

- 44,100 mixed frames
- 2,212,071 interpreted ARM instructions
- 42,912,864 idle-skipped ARM steps
- 4,403 Timer-A/FIQ events
- 0 ARM faults
- wall time ~1.06 seconds on the validation host

The same probe on 0.0.40.2 needed ~7.14 seconds and interpreted 45,125,632 ARM
instructions with zero idle-skipped steps.

## Windows item

WinMM output and physical keyboard latency cannot be acoustically validated in this
Linux environment. `run_homebrew_sfx_live.bat` is the Windows acceptance path. A
healthy 0.0.40.3 run should show `idle-skipped` growing rapidly, a much smaller
`prefill-ms`, and ideally `underrun-restarts=0` or a very small value.
