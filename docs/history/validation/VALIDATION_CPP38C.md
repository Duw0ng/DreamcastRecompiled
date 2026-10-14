# Validation — cpp38c SPG_WAIT_FAST_FORWARD

## Structural / ABI

- `VERIFY_CPP38C.py`: **15/15 PASS**.
- Guest ABI v1 structural fingerprint: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f` — **MATCH**.
- cpp38b2 guest implementation revision is retained: `cpp38b2-metricfree-v1`.
- SPG remains guest-cycle sampled; register reads do not directly advance raster time.

## Fast-forward safety

- Same-PC / same-SPG-value polling detection: PASS.
- Short-cycle guard (`<=256` cycles): PASS.
- Skip bounded to the remainder of one scanline: PASS.
- Pending render-done boundary clamp: PASS.
- Next PVR frame/VBlank boundary clamp: PASS.
- Pending Holly IRQ disables the skip: PASS.
- Host-sync boundary clamp: implemented.
- Runtime and generated emitter implementations match.

## Build / regression

- Runtime Debug build: **PASS, 100% compiled and linked** (`DreamcastRuntime`, Maple, GD-ROM and network targets).
- Core/top-level CTest run: **36/36 PASS**, including `cpp_emitter_tests` and CPU/codegen regression tests.
- Release validation compiled the modified emitter successfully and progressed through the wider build, but the full optimized build exceeded the execution window in this environment while compiling the very large runtime translation unit. No cpp38c compiler error was observed before that limit.

## Windows portable launcher

- `portable/DreamcastRecompiled.exe`: PE32+ Windows GUI x86-64.
- Launcher SHA-256: `e2243eb1b6fb8a8cf466406e12fab73ef94e672ee8216fe85e06bd17a60480ed`.
- Default environment enables `DCR_SPG_WAIT_FF=1`.

## Expected heartbeat

```text
build=v1.2-cpp38c-spg-wait-fast-forward
spg=...
spg-ff=on/HITS/CYCLES/MAX_CHUNK/0xLAST_PC
fpu-mode=superblock
fpu-metrics=off
host-sync-q=262144
```

For an A/B run, launch with `--spg-ff-off`. The important comparison is FPS together with `spg` read count, `spg-ff` hits/cycles, PVR MMIO reads, producer gap, audio starves, PVR packets/triangles and frame cadence.
