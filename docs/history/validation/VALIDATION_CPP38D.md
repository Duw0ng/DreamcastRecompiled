# Validation — cpp38d HOT_DISPATCH_PROFILER

- `VERIFY_CPP38D.py`: **12/12 PASS**.
- Runtime-only Linux Debug build: **PASS**, including `DreamcastRuntime`, GD-ROM, Maple and Network plugins.
- Guest ABI v1 header is byte-for-byte identical to cpp38c FIX1. Raw SHA-256: `6eb0241c7cc8aac411d77145ff05b436a6cbd9a1fb64fae25ddcac38b22c2b10`.
- `TEST_CPP38D_HOTPC.bat` uses `--reuse`; it does not request `--perf` or guest recompilation.
- Profiler state lives outside `DCRuntime`, so no ABI layout field was added.
- Default sample stride: 2048 full scheduler ticks; warmup: 5 heartbeats; report cadence: every 5 heartbeats.
- cpp38c SPG wait fast-forward remains enabled.

Expected live output:

```text
[DCR38D HOTPC] samples=... stride=2048 warmup-hb=5 | ctx=0x...:N@PCT,... | func=0x... | pr=0x...
```
