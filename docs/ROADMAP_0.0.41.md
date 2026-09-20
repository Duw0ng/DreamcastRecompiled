# DreamcastRecomp roadmap — after 0.0.41

The project now advances on two permanent tracks: the complete KallistiOS demo corpus for deterministic hardware regressions, and ChuChu Rocket! as the first native Katana commercial target.

## 0.0.41 — raw commercial recompilation (current)

- CDI -> IP.BIN / 1ST_READ.BIN local extraction.
- Symbol-free function closure discovery.
- Synthetic function map -> CFG -> DCIR -> C++.
- P1/P2 code alias canonicalization.
- Build the generated commercial C++ project and execute until the first real bootstrap dependency.
- Preserve the 0.0.40.4 dedicated low-latency audio worker; use telemetry to distinguish producer stalls from Windows device stalls.

## 0.0.42 — Katana boot environment

- Model the state normally established before control reaches 1ST_READ.BIN.
- Identify and seed/implement system RAM structures only from observable Dreamcast/Katana behavior, avoiding game-specific address hacks.
- Expand SH-4 exception/VBR/interrupt startup behavior.
- Turn raw-runtime failures into named bootstrap dependencies and permanent tests.
- Goal: pass the current early Katana initialization slice and reach the next subsystem boundary.

## 0.0.43 — GD-ROM/G1 and filesystem path

- G1/GD-ROM MMIO and DMA lifecycle needed by Katana gdFs.
- Mount the user's CDI as the runtime disc source without embedding commercial files in distributed output.
- Implement/read-track data path sufficiently for the game to request its first assets.
- Goal: observe successful commercial file reads from the original CDI.

## 0.0.44 — Holly interrupts + commercial Maple

- Complete asynchronous SH-4 interrupt entry/return path needed by Katana.
- Holly event masks/ack semantics under the commercial runtime.
- Maple DMA packets rather than only KOS-facing host bridges.
- Goal: controller enumeration and input through the game's own Katana path.

## 0.0.45 — commercial PVR bootstrap

- Follow Kamui/PVR register setup and TA list submission from the commercial runtime.
- Convert the status window into the actual Dreamcast video surface for the commercial executable.
- Goal: first non-placeholder ChuChu Rocket! frame or display clear produced by the game's own render path.

## 0.0.46 — assets + scene/menu

- Texture formats and render-state gaps discovered from ChuChu rather than guessed in advance.
- Resolve remaining GD-ROM streaming and synchronization issues.
- Goal: stable title/menu presentation.

## 0.0.47 — commercial AICA + gameplay loop

- Route the game's own sound driver through ARM7/AICA.
- Fix remaining audio producer pacing without reintroducing large latency buffers.
- Goal: menu audio + controls + transition into a game board.

## 0.0.48+ — playable target and recompilation hardening

- First complete playable ChuChu Rocket! gameplay slice.
- Save/load, timing, controller edge cases and deterministic regression captures.
- Reduce runtime interpretation/compatibility shims where static recompilation can replace them safely.
- Generalize every commercial fix into Dreamcast/Katana behavior before considering a second game.

## Rules for progress

- Never ship commercial game bytes in DreamcastRecomp archives.
- The user supplies their own CDI locally.
- Do not add a ChuChu-specific address hack when the real dependency can be represented as Dreamcast/Katana hardware or boot state.
- Every fixed blocker becomes a regression test or reproducible probe.
- KallistiOS demos remain the broad hardware test corpus while ChuChu provides the commercial reality check.
