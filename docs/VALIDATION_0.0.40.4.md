# Validation 0.0.41

## Core automated suite

Linux build completed successfully and:

```text
ctest --test-dir build-linux --output-on-failure
47/47 PASS
```

A freshly generated standalone project from the supplied real KallistiOS `sound/sfx/sfx.elf` also configures, builds, and its `generated_compile_test` exits successfully.

## Real deterministic SFX regression

The generated 0.0.41 SFX runner was executed on Linux with real ROMFS assets, standard `stream.drv`, native ARM7/AICA, deterministic device clock, and automatic controller A -> Start.

Observed:

```text
Reachable functions: 182
native-starts:       1
mix-frames:          3302
nonzero:             2696
ARM faults:          0
unsupported formats: 0
bad AICA reads:      0
```

The Windows audio worker is not active in this deterministic Linux run, so the guest PCM path remains independent of the new host-delivery policy.

## Full supplied KallistiOS corpus

The exact supplied 155-ELF KallistiOS corpus was re-scanned with 0.0.41:

```text
ELF found:                    155
ELF loaded:                   155
ISA-clean (all functions):    155
symbolized functions scanned: 202431
known SH-4 instructions:      17828452
unknown SH-4 instructions:    0
_main call graphs scanned:    155
_main with RAW_SH4=0:         155
```

This keeps **all** demos as the permanent regression corpus, not only the currently interesting SFX/PVR examples.

## ChuChu Rocket! commercial baseline

The newly supplied CDI is recognized by the 0.0.41 disc path and its real `1ST_READ.BIN` is extracted locally. The new `dc_raw_boot_probe` resolves the first raw indirect bootstrap jump:

```text
0x8C01001C -> 0x8C0DA540
```

A bounded deep discovery run reached 4096 candidate blocks and 31,540 unique instruction words, resolving 791 indirect transfers. Because raw data/code separation is not implemented yet, 412 words from that broad heuristic walk are classified as unknown/ambiguous rather than true ISA failures.

See `CHUCHU_ROCKET_BASELINE_0.0.41.md`.

## Windows acceptance still required

The dedicated WinMM worker cannot be acoustically validated in this Linux environment. The Windows acceptance run is:

```bat
run_homebrew_sfx_live.bat "C:\path\to\sfx.elf"
```

Primary target: `underrun-restarts=0`, with a small `winmm-gap-max-ms` even when `producer-gap-max-ms` shows a longer emulator stall.
