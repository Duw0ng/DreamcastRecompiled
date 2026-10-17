# DreamcastRecompiled v1.2 Rev 1 — Native AICA Foundation

Status: **experimental development branch**, not a stable release.

## Version policy

Keep the product name **DreamcastRecompiled v1.2**. Increment **Rev 1, Rev 2, ...**
only for discrete, testable revisions. Do not label ongoing AICA work as v1.3
or overwrite the public v1.2 Stable release. No PVR changes are included here.

## Rev 1 changes

- The ARM7 AICA bus now implements a 24-bit address decode consistent with the
  Dreamcast sound CPU: 2 MiB RAM mirrored across 0x000000–0x7FFFFF.
- Register access uses the remaining ARM7 address space and mirrors the 0x8000
  register window. Addresses like 0x200000 no longer fault simply because
  they lie outside the first 2 MiB RAM window.
- Shared `AICAArmRamView` read/write helpers handle 8/16/32-bit little-endian
  accesses, including operations crossing a 2 MiB mirror boundary.
- New CTest coverage checks RAM mirrors, MMIO selection, 24-bit aliasing, and
  an actual ARM7 firmware fragment fetched from mirrored RAM.
- Existing SH-4 guest ABI, existing AICA sound synthesis/ADPCM paths, GD-ROM,
  Maple, and PVR have not been intentionally modified.

## What Rev 1 is NOT

- **Not** an ARM7-to-x86-64 JIT or AOT compiler.
- **Not** a native DSP microprogram recompiler.
- **Not** a fix verified in Crazy Taxi 2, Mr. Driller, or Segagaga.
- **Not** a binary/portable release.
- The existing `arm7_step` interpreter still executes all ARM7 guest code.

## Acceptance

Build the standalone runtime and run:

```sh
cmake -S runtime -B build-rev1 -DCMAKE_BUILD_TYPE=Release
cmake --build build-rev1 --target dcr_aica_arm_bus_tests
ctest --test-dir build-rev1 -R dcr_aica_arm_bus_tests --output-on-failure
```

Then compare audio and AICA ARM faults against the v1.2 Stable baseline in
ChuChu Rocket!, Sega Tetris, Mr. Driller and Segagaga. The RAM fix may alter
behavior; successful CTest does not establish per-game correctness.

## Next audio-only revisions

1. **Rev 2 — ARM7 instrumentation and correct shared-clock scheduling.**
   Capture firmware image, ARM7 PC/opcode/flags/registers, MMIO and interrupt
   transitions in comparable interpreter traces; test sound timing and buffer
   underruns before optimization.
2. **Rev 3 — ARM7 native prototype.**
   Decode ARMv4T into an explicit IR; compile verified ARM basic blocks to
   executable x86-64 code. Implement CPU-mode switches, exception boundaries,
   block deadlines, and a native/interpreter differential harness. Native ARM
   execution is only enabled for proven opcodes.
3. **Rev 4+ — Coverage and cache safety.**
   Expand ARM/Thumb instruction coverage; safely invalidate code on SH-4 and
   ARM writes to translated pages, track firmware identity, and persist
   relocatable native output by firmware hash + backend version.
4. **Subsequent audio revisions — AICA DSP/fidelity.**
   Migrate AICA into `dcr_aica.dll` after a validated device-state/IRQ/buffer
   ABI; implement the full envelope/LFO path and a reference DSP engine
   before lowering DSP microprograms to native blocks.

The goal for enabling the native ARM7 backend by default is **no state
divergence in deterministic ARM7 execution traces**, no new ARM faults, and
audio waveform/tempo regressions meeting agreed tolerances in representative
games. Keep the interpreter only as a diagnostic comparison/fallback while
native coverage is incomplete.

## Notes for the native compiler

`runtime/include/dcr_aica_arm_memory.hpp` is a common semantic memory view,
not a native code cache. Before enabling translated firmware blocks, wire
page-generation counters/invalidation into **both** ARM and SH-4 writes to
AICA RAM (including bulk DMA), and sample IRQ/FIQ at architectural boundaries.
AOT or JIT code that bypasses these requirements can silently corrupt audio.
