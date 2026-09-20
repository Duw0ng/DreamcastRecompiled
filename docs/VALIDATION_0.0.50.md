# DreamcastRecomp 0.0.50 — validation

## Unit/regression gate

Final release gate: **47/47 CTest PASS**.

## Commercial ChuChu Rocket! gate

Static commercial closure:

- reachable functions: **2,194**;
- known SH-4 instructions: **165,555**;
- unknown SH-4 instructions: **0**;
- `RAW_SH4`: **0**.

Runtime acceptance at the intentional 1,200-packet diagnostic boundary:

- TA packets: **1,200**;
- vertices: **853**;
- triangles: **410**;
- sprites: **15**;
- `TA_LIST_INIT`: **38**;
- list-end Holly events: **93**;
- guest `STARTRENDER`: **31**;
- total render starts: **32** (31 guest + 1 initial heuristic bridge);
- render completions: **32**;
- page flips: **31**;
- framebuffer read-base writes: **68**.

The probe terminates only because the development packet limit is intentionally reached. Before that limit, Katana acknowledges the newly modeled list/render events and the completed framebuffer contains a clean SEGA splash.

## KallistiOS corpus

The existing 0.0.50 pass over the cached 155-ELF corpus remains valid because the final post-pass changes are runtime-only (Holly/PVR presentation, GD-ROM HLE and AICA scheduling), not decoder/CFG/DCIR changes. The large source ZIP was not re-extracted or redundantly scanned.

```text
ELF found / loaded:           155 / 155
ISA-clean:                    155 / 155
Known SH-4 instructions:      17,953,662
Unknown SH-4 instructions:    0
_main RAW_SH4=0:              155 / 155
```

Reports: `corpus/KOS_CORPUS_0.0.50.txt` and `.csv`.

## Audio / scheduling regression

The AICA/ARM7 clock domain is corrected to **22.5792 MHz**, which corresponds to **512 ARM clocks per 44.1 kHz mixed sample**. In commercial `--device-clock-host` mode, interpreted ARM7 catch-up is bounded to 4,096 steps per synchronization point; excess host-only debt is counted as `host-aica-drop` rather than starving SH-4/PVR/Win32. This is an explicit performance bridge until ARM7 is recompiled/JITed, not a claim of cycle-exact host synchronization.

Fresh real KOS `sound/sfx` regression after that change:

```text
runner RC=0
reachable functions=182
RAW_SH4=0
ARM7 faults=0
native-starts=1
mix-frames=3366
nonzero PCM frames=2602
unsupported AICA formats=0
bad AICA reads=0
```

## Long-run Windows findings addressed

The 0.0.49 long run exposed `GD-ROM HLE command 0x15` and severe host-window starvation while ARM7 accumulated billions of interpreted instructions. 0.0.50 models the CDDA command-state family (`0x14`/`0x15`/`0x16`/`0x17`/`0x21`), pumps PVR window messages during common runtime and ARM7 slices, removes frame-sync sleeping, corrects the AICA clock, and bounds host catch-up. Heartbeat now exposes `host-aica-drop` so the tradeoff is visible rather than hidden.

## Packaging

The release archive must exclude `build/`, `generated/`, CDI contents, `IP.BIN`, `BOOTSTRAP.BIN`, commercial generated C++, logs and diagnostic framebuffer dumps. Exact duplicate file contents are checked by SHA-256 before packaging.
