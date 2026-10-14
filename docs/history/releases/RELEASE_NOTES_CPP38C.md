# Release notes — cpp38c runtime-only fix

- SPG WAIT FAST-FORWARD remains a runtime-only optimization.
- Existing Guest ABI1 caches are now strictly immutable during `--reuse`.
- Removed the inherited cpp38b2 one-time metric-free guest migration from the cached-guest path.
- `--reuse` no longer builds `dc_raw_recomp`; the packaged `dc_disc_probe` and `dc_boot_prepare` are used directly.
- A missing/invalid ABI1 cache causes `--reuse` to abort instead of compiling SH-4.
- Runtime/plugins may rebuild once for build id `cpp38c-spg-wait-fast-forward`; this does not configure, compile or relink `dcr_game_module.dll`.
- Window/status titles now identify `cpp38c`.
