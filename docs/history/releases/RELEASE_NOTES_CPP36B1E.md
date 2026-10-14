# DreamcastRecompiled v1.2 cpp36b1e — RUNTIME_EXTERNAL_RAW_ENV

This build fixes the portable-safe guest path without recompiling SH-4. Older cached `dreamcast_program.exe` binaries do not understand the newer `--external-raw-image` runner option, so cpp36b1e moves that handoff into `DreamcastRuntime.dll`.

## Expected log

```text
[PORTABLE] Bootstrap externo via Runtime ENV: ...\BOOTSTRAP.BIN @ 0x8C008000
[DCR PORTABLE] external raw image loaded by Runtime ENV @0x8C008000 bytes=...
[DCR BOOT] commercial executable kept plain @0x8C008000 (...)
```

The cached Guest ABI1 module is reused unchanged. `--reuse` may rebuild only Runtime/plugins when the runtime build ID changes, then auto-syncs the portable package.
