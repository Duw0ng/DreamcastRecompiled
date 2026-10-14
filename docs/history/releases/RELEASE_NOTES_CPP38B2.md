# Release notes — cpp38b2

cpp38b2 fixes the cpp38b1 telemetry bug found from the Windows log: `fpu-mode=superblock` still forced `fpu-metrics=on` even when `DCR_FPU_METRICS=0`.

The production modular guest now compiles generated FPU telemetry out with `DCR_FIXED_FPU_REGION_METRICS=0`. Superblock execution, index-scratch PVR path and 262144 host-sync remain unchanged so this build isolates telemetry overhead as cleanly as possible.

A valid ABI1 cache is migrated and rebuilt at the generated-C++ level once; SH-4 closure discovery is not repeated. Guest ABI1 layout remains frozen.
