# DreamcastRecomp roadmap — after 0.0.47

## 0.0.47 — checkpoint complete

Completed generically, without ChuChu-specific address seeds:

- overlapping/shared-tail raw SH-4 closure based on CFG rather than synthetic next-symbol bounds;
- preservation of long shared epilogues and delay-slot restores;
- conservative callback discovery for tail-JMP argument flow, return-before-literal-pool callbacks, `RTS; NOP` no-op callbacks and packed BRA task-entry thunks;
- non-local/context-switch return propagation in generated CALLs;
- host-clock PVR VBlank scheduling independent of MMIO polling;
- PVR progress counters in the heartbeat;
- 47/47 CTest and the exact 155-ELF KallistiOS baseline remain clean;
- real ChuChu acceptance advances through GD-ROM and task switching for a 40-second diagnostic window without a runtime error.

## 0.0.48 — first priority: commercial TA/Kamui submission

The current retail trace advances VBlank but still reports zero TA packets/renders. Continue from that demonstrated boundary:

1. Instrument PVR/TA MMIO writes and Store Queue commits by phase so it is clear whether ChuChu configures the TA but never submits, or never reaches configuration at all.
2. Track the TA list-init/list-cont/end-of-list/render-start lifecycle and Holly pending/mask bits used by the Katana path.
3. Verify Store Queue address translation and QACR handling for the exact commercial path before adding any new alias.
4. If the game is blocked before TA submission, identify the wait condition (Holly event, VBlank phase, GD-ROM completion, Maple, or scheduler semaphore) and implement the reusable hardware behavior it expects.
5. Acceptance: first non-zero commercial TA packet/list and then first render-start, while unknown SH-4 and `RAW_SH4` remain zero.

## Parallel priority: commercial AICA firmware/reset sequence

The 0.0.46 diagnostics showed that the retail path can release ARM7 with blank AICA RAM/vector state and linearly execute zeros to `0x00200000`. Keep the no-wrap behavior and determine where the Katana sound driver/firmware is supposed to be uploaded or initialized. Do not infer mirroring merely from the final PC.

Acceptance: a retail ARM7 release with a populated program/vector image and meaningful execution, or a documented/trace-proven reason the title intentionally keeps ARM7 reset/blank during the current boot phase.

## Permanent regression gates

Every commercial fix must preserve:

- core CTest suite;
- all 155 supplied KallistiOS ELF files ISA-clean;
- all 155 `_main` graphs at `RAW_SH4=0`;
- standard KOS `sound/sfx` native ARM7/AICA path;
- Maple host-input path;
- representative PVR demos;
- commercial acceptance trace progressing at least as far as the previous milestone.

Keep the architecture static/generated-native: missing hardware behavior belongs in reusable Dreamcast services, not in a runtime SH-4 interpreter fallback or title-address patches.
