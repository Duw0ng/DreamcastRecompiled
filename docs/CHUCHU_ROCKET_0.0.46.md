# ChuChu Rocket! commercial status — 0.0.46

0.0.46 addresses the two concrete boundaries left by 0.0.45: the dense indexed SH-4 callable family reached from `0x8C0FD600`, and the ARM7 path that previously ran to `0x00200000` without enough evidence to distinguish bad/missing firmware state from a true mapping requirement.

## Dense indexed SH-4 dispatch

The previously inspected retail path uses a table around `0x8C185D58` with callable entries including:

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

0.0.46 does **not** whitelist these addresses. `analyze_function()` now recognizes the reusable instruction shape in which a PC-relative literal gives the absolute table base, an indexed `MOV.L @(R0,Rn),Rn` loads an entry, and `JSR @Rn` or `JMP @Rn` dispatches through it.

Local/basic-block `JMP` targets keep the old behavior. Cross-function targets, and every `JSR` table entry, must pass a stronger callable check: an aligned executable address with a clean 16-halfword / 32-byte known SH-4 prefix. This intentionally allows long functions without requiring an early return.

The raw symbol-free recompiler consumes these dynamic targets and promotes them into closure when they are local, have the same clean prefix, and are not themselves pointer tables. The closure report now includes:

```text
Dense dispatch targets:        N
```

On the same retail image/trace, a non-zero count around the previous boundary is the first confirmation that the family is being promoted.

## ARM7/AICA diagnosis without fake mirroring

0.0.46 does not paper over the old `pc=0x00200000` failure by masking the PC into 2 MiB. Instead, the runtime captures the actual AICA RAM state when ARM7 reset is released and tracks what the ARM7 executes afterward.

The final `[AICA ARM7]` line now includes fields such as:

```text
release=
sh4-aica-writes=
sh4-aica-bytes=
release-nonzero=
release-first-nz=
pc-high=
zero-opcodes=
zero-run-max=
```

If fetch reaches the first byte beyond AICA RAM, the detailed error also prints vector words from the release snapshot and can append:

```text
reset-vector-blank-at-release
long-zero-opcode-linear-fallthrough
```

Interpretation:

- very small/zero `release-nonzero` plus blank vector words strongly suggests the ARM7 was released before a valid program/vector image was installed;
- a very large `zero-run-max` together with `pc-high=0x200000` indicates linear execution through zero-filled RAM;
- populated vectors/program RAM with a non-linear execution history would instead justify investigating an actual address-map/control-flow rule.

## Windows acceptance run

From a normal 0.0.46 Windows build:

```bat
run_commercial_recompiled.bat "RUTA\ChuChu Rocket!.cdi"
```

The useful tail for the next iteration is:

1. `Dense dispatch targets:` from the recompile/closure summary;
2. the final `[AICA ARM7]` statistics line;
3. the last `[CALL]` / `[DreamcastRecomp ERROR]` line if execution still stops.

The external CDI was not available in the clean packaging workspace, so this document describes the implemented acceptance criteria rather than claiming a retail pass that was not run.
