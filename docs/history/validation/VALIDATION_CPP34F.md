# VALIDATION CPP34F

- Source verifier: PASS.
- Linux CMake build of `DreamcastRuntime`: PASS.
- Linux CMake build of `dcr_network`: PASS.
- Cached runner banner migration from cpp34d: PASS.
- Cached runner banner migration from cpp34e: PASS.
- Tick path is observe-only: PASS.
- Live `relocated_code_aliases` check executes before any IP.BIN restore: PASS.

## Regression addressed

cpp34e correctly restored the initial `0x8C00FA00` / `0x8C00F400` relocation discovery,
but after the first PVR flip it could arm while already dispatching `0x8C00FA00` and
immediately overwrite that live relocated helper with the immutable IP.BIN shadow.

cpp34f never arms from an IP.BIN target and never restores over a live relocation alias.
