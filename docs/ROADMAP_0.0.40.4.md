# DreamcastRecomp roadmap — after 0.0.41

The project now has two equally important acceptance tracks:

1. **general Dreamcast behavior**, guarded by the full 155-demo KallistiOS corpus;
2. **real commercial progress**, guarded by the supplied native ChuChu Rocket! CDI.

A release should not be considered progress merely because one hand-picked demo improves. Shared hardware/runtime fixes must keep the corpus green and move the commercial target farther toward execution.

## Current baseline — 0.0.41

### SH-4 / recompiler

- 155/155 supplied KallistiOS ELFs load.
- 17,828,452 symbolized SH-4 instructions decode with 0 unknowns.
- 155/155 `_main` graphs are `RAW_SH4=0`.
- Generated C++ regression suite remains 47/47 PASS.

### AICA / ARM7

- Standard KOS `stream.drv` runs through the real ARM7 interpreter.
- `sound/sfx` reaches real AICA slot key-ons and audible WinMM output on Windows.
- ARM7 polling-loop fast-forward removes the dominant standard-firmware interpreter waste.
- 0.0.41 adds a dedicated WinMM worker and bounded PCM ring so host audio delivery is no longer directly coupled to SH-4 scheduling.

### Maple / Holly

- Standard controller host input works through KOS-facing Maple calls.
- Low-level Maple GETCOND/DEVINFO and DMA completion are modeled.
- Holly pending/ack bit for Maple DMA exists.
- Full asynchronous SH-4 interrupt entry/priority delivery is still missing.

### PVR

- Existing KOS PVR path includes TA submission, software rasterization, texture uploads, palette formats, bilinear, sprites, VQ, RTT and a host PVR window.
- The next visual milestone is not another decorative status window: the commercial runner must eventually present the actual Dreamcast scanout/framebuffer/PVR result.

### ChuChu Rocket!

- CDI -> ISO9660 -> IP.BIN -> `1ST_READ.BIN` works.
- Raw load base is `0x8C010000`.
- Symbol-free raw discovery resolves the real initial bootstrap jump `0x8C01001C -> 0x8C0DA540`.
- The bounded deep probe reaches thousands of candidate raw blocks and hundreds of indirect targets.
- The normal C++ recompiler still requires ELF symbols; raw data/code separation and synthetic function discovery are the immediate blocker.

---

## 0.0.41 — raw commercial recompiler pipeline

**Goal:** compile a useful portion of ChuChu Rocket! directly from its raw boot binary, without converting the game into a hand-authored ELF and without title-specific function lists.

Pipeline:

```text
CDI/GDI
 -> disc/session reader
 -> ISO9660
 -> IP.BIN
 -> boot binary
 -> descramble detection when required
 -> raw image @ 0x8C010000
 -> symbol-free code/data discovery
 -> synthetic code units / call graph
 -> existing DCIR
 -> generated C++
 -> commercial native runner
```

Required work:

- promote raw bootstrap discovery into a reusable `RawProgramImage` / `RawFunctionDiscovery` layer;
- track basic-block state across direct control flow rather than a purely local heuristic;
- recognize literal pools and probable jump tables separately from executable words;
- discover function roots from BSR, resolved JSR/JMP, address-taken code pointers and return boundaries;
- give synthetic functions stable names such as `sub_8C0DA540`;
- let the existing emitter embed raw image segments without ELF section tables;
- add exact diagnostics for the first missing runtime target/MMIO address.

**ChuChu acceptance:** a generated C++ project is produced from the CDI/boot and executes the real startup until the first genuine hardware/service blocker. The blocker must be reported with guest PC and operation; a silent unknown indirect call is not acceptable.

---

## 0.0.42 — real startup + SH-4/Holly interrupts

**Goal:** remove demo-specific pre-main assumptions and make the commercial bootstrap behave like a Dreamcast process.

Priorities:

- SH-4 exception/interrupt entry, VBR/SSR/SPC/SGR state and `RTE` path;
- Holly interrupt masks/priorities/pending banks;
- asynchronous Maple DMA completion delivery;
- timer events needed by Katana runtime startup;
- formal BIOS/syscall interception layer where the game genuinely calls firmware services;
- migrate away from `--probe-kos-aica-defaults` as real startup state becomes modeled.

**ChuChu acceptance:** startup proceeds through the first Katana initialization phase rather than stopping at an interrupt/service boundary.

---

## 0.0.43 — GD-ROM / G1 / filesystem path

**Goal:** make the commercial game read its own data after boot.

Priorities:

- GD-ROM command/status model;
- sector reads from the supplied local disc image;
- G1/DMA behavior used by the SDK;
- enough `gdFs` behavior to reach the first real asset/configuration read;
- deterministic disc-read tracing (`LBA`, size, destination, completion event).

**ChuChu acceptance:** observe and satisfy the first real post-boot game data read from the CDI.

---

## 0.0.44 — first real commercial video frame

**Goal:** the host window becomes Dreamcast video output, not merely a liveness/status window.

Priorities are driven by whatever ChuChu actually uses:

- exact TA list/control words encountered by the game;
- missing ISP/TSP state;
- framebuffer/scanout registers and page flipping;
- texture formats/blend/depth behavior exposed by commercial assets;
- VBlank timing linked to Holly interrupt delivery.

**ChuChu acceptance:** first recognizable commercial frame, then title/Press Start image.

---

## 0.0.45 — commercial audio/input/peripherals integration

**Goal:** keep the already working generic AICA/Maple paths valid under the commercial SDK runtime.

- validate Katana `sd` audio against native AICA/ARM7 rather than inventing a game-specific sound bridge;
- controller input through the game's actual Maple/pd path;
- VMU/rumble only when encountered, implemented generically;
- keep WinMM worker latency telemetry active during commercial execution.

**ChuChu acceptance:** title/menu responds to a real controller and commercial audio reaches the host.

---

## 0.0.46 — menu / board initialization

**Goal:** move beyond a title-screen proof.

- menu transitions;
- game-state allocations;
- stage/board data load;
- any missing PVR or system-service behavior found here becomes a reusable regression.

**ChuChu acceptance:** enter a playable mode and display a board correctly.

---

## 0.0.47 — first playable round

**Goal:** one complete local gameplay loop through recompiled commercial code.

Acceptance includes:

- controller movement/action;
- stable video/audio timing;
- no title-ID patches;
- no emulator-core fallback for normal SH-4 code;
- clean restart/exit path;
- newly discovered generic behaviors covered by independent tests/demos.

---

## Permanent regression matrix

Every meaningful release should report at least:

```text
Core CTest                 PASS/PARTIAL/REGRESSION
155 KOS ELF corpus         loaded / ISA clean / RAW_SH4
2ndMix                     audio + PVR + clock regression
sound/sfx                  standard ARM7/AICA + live latency
sfxbuf / streaming audio   buffer/stream regression
PVR palette demos          4bpp / 8bpp / wormhole
Plasma                     SQ texture upload + filtering
Texture Render             RTT + VRAM reuse
Bumpmap                    sprite/VQ/bump path
PVRMark                     frontend/stress
Raytris / GL               game-like integration
Maple controller           host + DMA
VMU / rumble / KB / mouse  as implemented
Thread/timer demos          scheduler/interrupt work
ChuChu Rocket! CDI         commercial milestone reached
```

The commercial CDI itself remains a local user-owned test input and is never packaged with DreamcastRecomp.

## Rules for commercial progress

- No game-ID/address hacks to make ChuChu skip initialization.
- A discovered hardware behavior should be implemented at the Dreamcast/Katana layer and then protected by a small regression when practical.
- Unknown raw words are not automatically treated as missing SH-4 opcodes until code/data separation proves they are executable.
- Demos remain mandatory because they isolate failures; the commercial title remains mandatory because it exposes integration failures that demos do not.
