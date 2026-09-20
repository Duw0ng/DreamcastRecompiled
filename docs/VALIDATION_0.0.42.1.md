# DreamcastRecomp 0.0.42.1 — commercial bootstrap trace fix

## Root cause found from the Windows report

The repeated sequence around `0x8C008EFA -> 0x8C008AD0` is not a hardware wait loop.
The IP.BIN bootstrap is decoding an `MR`-tagged graphics stream. The decoder at
`0x8C008D74` checks the bytes `M` (`0x4D`) and `R` (`0x52`), and its inner path
calls `0x8C008AD0` repeatedly to place decoded pixels/words in the destination
surface. `0x8C008F70` is another small decoder helper.

In 0.0.42 `run_commercial_recompiled.bat` enabled `--trace-calls`. That meant a
normal bootstrap operation produced tens of thousands of `[CALL]` and `[RET]`
lines. Console I/O, especially a visible Windows console, became the dominant
cost and made a finite decoder loop look hung.

The user's manual close was reported as RC=0 because host-window close was
considered a normal exit for the homebrew live runners. That was misleading for
a commercial boot test.

## Fixes

- Full `--trace-calls` is no longer enabled by the commercial BAT by default.
- The runtime always records a rolling history of the last 32 calls.
- On a runtime error, those 32 calls are printed automatically before the error.
- Full `--trace-calls` remains available for explicit deep debugging.
- Closing the host status window during `--commercial-boot` now returns RC=130
  and explicitly reports that the bootstrap was aborted by the user.

## Local validation with the supplied ChuChu Rocket! CDI

Commercial raw recompilation remains unchanged:

- reachable functions: 806
- reachable SH-4 instructions: 53,925
- known SH-4: 53,925
- unknown SH-4: 0
- RAW_SH4: 0
- CFG blocks: 9,486
- inline BSRF thunks: 11

The generated native runner reaches the same intended 0.0.42 frontier without
full trace spam. On the Linux validation host it reaches that frontier in about
0.47 s of execution after the generated program has been built.

The final runtime boundary remains:

`0x8C00DBE0 -> 0x8C001006`

with BIOS GD-ROM command state:

- r4 = 0x1E
- r5 = 0x8CFFFFF4
- r6 = 0
- r7 = 0

This is still the correct starting point for 0.0.43 GD-ROM HLE.

## Regression

`ctest`: 47/47 passed.
