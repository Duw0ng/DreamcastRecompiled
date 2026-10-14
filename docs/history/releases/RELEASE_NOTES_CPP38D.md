# Release notes — cpp38d

- Adds a low-overhead live SH-4 hotspot profiler implemented entirely in DreamcastRuntime.
- No Guest ABI changes and no generated guest rebuild is required.
- Samples `ctx.pc`, `runtime.current_pc`, and `PR` every 2048 full ticks after a 5-heartbeat warmup.
- Reports the top eight addresses every five heartbeats in `[DCR38D HOTPC]`.
- Keeps cpp38c SPG wait fast-forward and all cpp38b2/cpp38c behavior unchanged.
- `TEST_CPP38D_HOTPC.bat` forces `--reuse` and never requests guest recompilation.
