# cpp38d — HOT_DISPATCH_PROFILER

Runtime-only profiler layered on cpp38c. It preserves Guest ABI v1 and reuses an existing guest module byte-for-byte.

The profiler samples SH-4 execution every 2048 full scheduler ticks after a five-heartbeat warmup and reports cumulative top PCs every five heartbeats:

- `ctx`: exact `SH4Context::pc` observed at the scheduler boundary.
- `func`: runtime `current_pc` / current generated function context.
- `pr`: guest return address, useful for identifying the caller around a hot leaf.

It does not enable the heavy `--perf-profile` timer instrumentation and does not rebuild or patch generated guest C++.

Expected line:

```text
[DCR38D HOTPC] samples=... stride=2048 warmup-hb=5 | ctx=0x...:N@PCT,... | func=... | pr=...
```

`SPG_WAIT_FAST_FORWARD` from cpp38c remains enabled by default.
