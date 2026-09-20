# DreamcastRecomp roadmap — after 0.0.45

The development rule remains: use the complete supplied KallistiOS demo tree as broad regression coverage, while the supplied retail ChuChu Rocket! CDI drives the next real hardware/runtime dependency. Do not hide boundaries with title-specific address patches.

## 0.0.45 — checkpoint complete

Completed:

- reachable CFG fragments promoted back into symbol-free closure discovery;
- VBR-installed callback discovery;
- callback objects with small scalar headers;
- short-thunk vs pointer-table discrimination;
- sparse one-level init/callback table discovery;
- SH-4 `FPSCR.SZ=1` FMOV support for DR and XD pairs;
- commercial closure increased from 907 to 1,453 functions while retaining 0 unknown SH-4 and RAW_SH4=0;
- retail execution advanced from `0x8C1414DE` to `0x8C138620` after about 12.67 million guest calls and two real CDI sector reads;
- 47/47 core tests and 155/155 KallistiOS ISA/DCIR regression remain clean.

## 0.0.46 — dense indexed dispatch tables + ARM7 commercial diagnosis

Primary SH-4 closure work:

- recognize a dense local function-pointer table when an indexed `JSR/JMP @Rn` is fed from a PC-relative table base;
- allow long callable targets when the table evidence is strong, using a clean multi-instruction SH-4 prefix rather than requiring an early return;
- promote the complete `0x8C185D58` dispatch family generically and rerun until the next true boundary;
- keep code/data discrimination measurable (`unknown=0`, `RAW_SH4=0`).

Parallel commercial AICA work:

- determine why ARM7 advances to the first byte beyond its 2 MiB addressable RAM (`0x00200000`);
- distinguish missing firmware/control-flow setup from address mirroring or a missing ARM7 halt/loop condition;
- do not mask the fault by silently wrapping arbitrary instruction fetches.

## 0.0.47 — first stable commercial frame path

Once the closure stays alive long enough:

- audit TA packet/list traffic from the retail game rather than only logical render starts;
- ensure CH2 DMA, PVR list completion and Holly IRQ ordering are sufficient for the real Katana path;
- present the retail framebuffer/PVR output in the Windows PVR window;
- capture deterministic frame/VRAM diagnostics for regression.

Target: first recognizable ChuChu boot/title imagery.

## 0.0.48 — commercial Maple + menu interaction

- validate the game's own Maple DMA descriptors/GETCOND path;
- map keyboard/XInput to the retail controller state without KOS-specific shortcuts;
- reach and navigate the title/menu flow.

## 0.0.49 — commercial AICA/audio integration

- boot the retail ARM7/AICA program without faults;
- validate G2/AICA RAM transfers, timers/interrupts and SDK `sd` behavior;
- route stable commercial PCM through the dedicated low-latency host audio path.

## 0.0.50+ — first gameplay slice

- load a real level/board from the CDI;
- render a stable board;
- accept player input;
- maintain sound and timing;
- reach a minimally playable native Windows slice before broad compatibility expansion.

## Permanent regression gates

Every commercial change should preserve:

```text
47/47 CTest PASS
155/155 supplied KallistiOS ELF ISA-clean
155/155 _main RAW_SH4=0
commercial closure unknown SH-4=0
commercial closure RAW_SH4=0
```

When a more aggressive closure heuristic violates these gates or starts interpreting data as code, reject it and prefer the smaller explicit boundary.
