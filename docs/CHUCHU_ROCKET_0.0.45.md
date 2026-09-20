# ChuChu Rocket! commercial status — 0.0.45

0.0.45 is a deeper symbol-free-closure and SH-4 FPU semantics checkpoint. The retail CDI remains external and is required only for local generation/testing.

## Boundaries passed since 0.0.44

The Windows-validated 0.0.44 stop was:

```text
0x8C1090FA -> 0x8C1414DE
```

0.0.45 promotes reachable CFG fragments back into closure discovery, so the literal target inside that fragment becomes a real generated entry. Subsequent local runs exposed and then passed several more generic cases:

1. VBR-relative callback installation, including `0x8C107E16`.
2. Callback objects with a small scalar/header prefix, including the `0x8C0EE158` family.
3. Tiny SH-4 thunks whose nearby literal pool previously resembled a pointer table, including `0x8C106D20`.
4. Sparse init/callback tables reached through one local data indirection, including the `0x8C110040` family.
5. `FPSCR.SZ=1` FMOV using odd encoded registers, which are XD register pairs from the opposite FPU bank.

None of these fixes contain a ChuChu-specific address whitelist.

## Current commercial progress

The final closure contains 1,453 functions and 83,588 reachable instructions with zero unknown SH-4 and zero RAW_SH4.

The current execution reaches roughly 12.67 million guest calls. BIOS GD-ROM HLE has processed seven requests and completed two real 2048-byte sector reads from the user's CDI before the next static closure boundary.

The current boundary is:

```text
[CALL] 0x8C0FD600 -> 0x8C138620
[DreamcastRecomp ERROR] No recompiled/native target registered for Dreamcast address 0x8C138620
```

`0x8C138620` is genuine SH-4 code. The call site indexes a dense function table whose base is around `0x8C185D58`. The first entries point at a family beginning:

```text
0x8C138620
0x8C138724
0x8C138920
0x8C138A84
0x8C138C20
0x8C138DA0
0x8C1390A0
0x8C139380
0x8C139620
```

The existing callback-entry predicate expects an early return/branch and therefore rejects long straight-line functions such as `0x8C138620`. The next release should use the strong evidence of a dense indexed dispatch table while still requiring a clean SH-4 prefix, instead of globally relaxing code/data discrimination.

## Separate ARM7 issue

The retail AICA/ARM7 path still runs into `0x00200000`, records one unmapped-access fault and halts. Commercial sound is therefore not considered functional yet. This can be investigated independently from the current SH-4 dispatch closure.

## Test on Windows

```bat
build_windows.bat
run_commercial_recompiled.bat "RUTA\\ChuChu Rocket!.cdi"
```

A matching 0.0.45 Windows baseline should pass the old `0x8C1414DE` failure and eventually stop at `0x8C138620`, unless platform timing exposes a different real dependency first.
