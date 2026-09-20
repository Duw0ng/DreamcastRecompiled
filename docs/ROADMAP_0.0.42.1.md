# DreamcastRecomp roadmap — after 0.0.42.1

## Guiding rule

The project advances on two permanent tracks at the same time:

1. the complete supplied KallistiOS demo corpus remains the broad hardware/ISA regression set;
2. a real retail Dreamcast title (currently ChuChu Rocket!) is executed regularly so the next missing subsystem is discovered by actual commercial code rather than by demo-only assumptions.

No commercial game bytes are shipped with DreamcastRecomp. Disc extraction and generated commercial C++ stay under the user's local `generated/` directory.

## 0.0.42.1 — real console bootstrap + compact diagnostics (current)

Completed:

- CDI -> ISO9660 -> IP.BIN + boot extraction.
- Automatic GD-ROM vs MIL-CD/self-boot boot preparation.
  - GD-ROM: preserve `1ST_READ.BIN` byte-for-byte.
  - self-boot: apply the historical 32-byte-slice descramble transform.
- Combined raw image layout:
  - `IP.BIN @ 0x8C008000`
  - canonical bootstrap entry `0x8C008300` (Dreamcast P2 execution alias `0xAC008300`)
  - game image `@ 0x8C010000`.
- Post-BIOS SH-4 state and low-RAM syscall vectors.
- 16 KiB mirrored on-chip/operand-cache RAM for the bootstrap work/stack area.
- P1/P2 code alias canonicalization.
- Symbol-free absolute jump-table discovery.
- Literal-fed PR continuation discovery.
- Runtime-selected local BSRF/RTS thunk-pack discovery.
- Continued native dispatch when the bootstrap returns to another Dreamcast PC.
- ChuChu static commercial closure: 806 emitted functions, 53,925 known SH-4 instructions, 0 unknown, RAW_SH4=0.
- ChuChu native execution reaches the first real BIOS GD-ROM call instead of failing at direct `1ST_READ.BIN` startup.
- KallistiOS regression: 155/155 ISA-clean and 155/155 `_main` RAW_SH4=0.
- Core regression: 47/47 CTest PASS.

Current commercial stop:

```text
[CALL] 0x8C00DBE0 -> 0x8C001006
[DreamcastRecomp ERROR] Dreamcast BIOS GD-ROM syscall reached before GD-ROM HLE is implemented
(r4=0x1E r5=0x8CFFFFF4 r6=0x0 r7=0x0)
```

That is the hand-off point for 0.0.43.


### 0.0.42.1 diagnostic correction

The IP.BIN MR graphics decoder performs tens of thousands of tiny helper calls.
The previous commercial BAT printed every CALL/RET, so Windows console output made
a finite software decode loop look stalled. Commercial runs now keep only a rolling
32-call history and dump it on failure. Manual close during commercial boot is RC=130.

## 0.0.43 — GD-ROM BIOS + disc data path

Primary goal: satisfy the bootstrap/game's real BIOS GD-ROM protocol using the same CDI supplied at launch.

Planned work:

- generated runner `--disc=<path>` / local disc attachment;
- BIOS GD-ROM command/request/status HLE;
- sector/FAD reads from the CDI/ISO payload;
- async request handles and completion polling expected by Katana;
- minimum G1/GD DMA behavior when the runtime crosses from BIOS calls to hardware registers;
- preserve the exact game-visible sector layout rather than fabricating file contents;
- telemetry: GD command, request id, FAD/LBA, sector count, destination, completion/status;
- run ChuChu until the first post-GD dependency is reached.

Acceptance target: the current `r4=0x1E` call completes correctly and the retail bootstrap reaches the next stage, ideally the transition into `1ST_READ.BIN` or its first filesystem request.

## 0.0.44 — Katana filesystem / interrupts / commercial Maple

Depending on the first 0.0.43 failure, prioritize the real dependency:

- gdFs/GD filesystem reads and seek behavior;
- Holly normal interrupt pending/mask/priority delivery;
- SH-4 exception/interrupt entry and return;
- Maple DMA command lists used by the Katana `pd` input stack;
- controller `GETCOND` through the retail path rather than only the KOS path.

Acceptance target: commercial startup can poll/read the disc and controller without host-only shortcuts.

## 0.0.45 — Kamui/PVR commercial frontend

- commercial TA list submission and Store Queue traffic;
- Kamui register initialization;
- render-start/render-done/vblank lifecycle;
- texture formats actually requested by ChuChu;
- make the existing Win32 host window the real Dreamcast video output for the retail path.

Acceptance target: first recognizable ChuChu frame, even if incomplete.

## 0.0.46 — title screen and menu

- finish PVR state exposed by the first frame;
- font/sprite/texture upload correctness;
- controller navigation through the commercial `pd` stack;
- stable vblank pacing;
- validate audio scheduling remains low-latency while PVR is active.

Acceptance target: title screen/menu can be displayed and navigated.

## 0.0.47 — commercial AICA / game audio

- Katana `sd`/AICA path rather than only KOS `stream.drv`;
- ARM7 firmware/data upload used by the retail SDK;
- DMA/timer/interrupt timing required by the game;
- keep the dedicated Windows audio worker and improve producer pacing without increasing interactive latency.

Acceptance target: menu/game audio from the commercial runtime with controller input and video active.

## 0.0.48+ — first playable board

- map/asset loading through the real disc path;
- gameplay update loop;
- collision/entity state;
- controller latency/pacing;
- save/VMU dependencies if encountered;
- eliminate remaining title-specific assumptions by converting them into reusable Dreamcast/Katana services.

Acceptance target: enter and play a ChuChu Rocket! board in the native generated executable.

## Permanent regression gates

A commercial fix is not accepted if it silently regresses the broad baseline. Each milestone should keep checking:

- core CTest suite;
- 155 KallistiOS ELF corpus;
- SFX/ARM7/AICA live path;
- Maple interactive path;
- representative PVR demos;
- the commercial bootstrap trace, always expected to fail later than the previous milestone.
