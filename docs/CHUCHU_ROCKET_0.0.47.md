# ChuChu Rocket! commercial status — 0.0.47

0.0.47 is based on the real 0.0.46 Windows trace and a local acceptance run against the same external ChuChu Rocket! CDI. No commercial bytes are shipped in DreamcastRecomp and no title-address seeds were added.

## 1. The `0x50000FAC` crash was a truncated shared epilogue

The 0.0.46 user trace stopped at:

```text
Dreamcast memory access is outside mapped RAM/VRAM/MMIO:
address=0x50000FAC physical=0x10000FAC width=2 guest_pc=0x8C0086DE
```

The failing pointer was downstream damage rather than a missing Dreamcast alias. The IP.BIN bootstrap contains overlapping/shared-tail dispatch entries around `0x8C009BFE`. One valid entry crosses another discovered entry before reaching its real `RTS` and the delay-slot `MOV.L @R15+,R3` that restores the stack/register state.

The old raw synthetic ELF bounded each function at the next discovered entry, so that epilogue was silently omitted. 0.0.47 gives each `.raw_boot` entry the complete executable envelope and lets CFG reachability plus actual SH-4 terminators determine the path. The same structural fix also covers much longer shared continuations such as the `0x8C101260` family, whose real callee-saved-register restore sits hundreds of bytes later.

The apparent later write through `0xC8800000` disappeared with this fix; it was a leaked/restoration-truncated `R11`, not evidence that a new MMU alias should be hard-coded.

## 2. Katana callback/task entry closure

Once those epilogues were preserved, interrupt-driven execution exposed real callbacks that are legal code but intentionally tiny or surrounded by data. 0.0.47 recognizes them only through generic argument/call data flow and conservative entry shapes:

- callbacks reached through an ABI-preserving `JMP` tail call;
- callbacks that reach a real `RTS/RTE` before an adjacent literal pool;
- canonical no-op callback `RTS; NOP`;
- packed task-entry thunks beginning with `BRA common_worker` and a valid delay slot, when the branch target itself has a clean executable prefix.

An intentionally broader heuristic that accepted arbitrary early branches was tested and rejected because it promoted data as code. The final rule adds only the demonstrated callback family while preserving zero unknown instructions and zero `RAW_SH4`.

Final static closure:

```text
Closure passes:             23
Reachable functions:        1630
Reachable instructions:     107868
Known SH-4:                 107868
Unknown SH-4:               0
RAW_SH4:                    0
Argument callbacks:         52
Dense dispatch targets:     77
```

## 3. Full task/context restore must be a non-local return

The retail scheduler then reached a full SH-4 context restore. That routine restores general/FPU/control registers, `PR`, and the saved task PC. The generated CALL path previously overwrote `ctx.pc` with the caller's ordinary return address after every callee completed.

That mixed the newly restored task registers with the previous task's PC and produced the false dynamic target `0x8C00F300`.

0.0.47 distinguishes the cases generically:

- ordinary generated/HLE call: return to the expected caller continuation;
- context restore/non-local return: if the callee leaves `ctx.pc` at another Dreamcast PC, propagate it to the dispatcher.

After this change the false `0x8C00F300` target disappears and execution resumes at the task entry selected by the restored context.

## 4. Host-clock VBlank

With the scheduler active, ChuChu could wait for interrupts without continuously polling PVR status. The previous host-clock path advanced AICA from wall time but only produced PVR VBlank as a side effect of selected MMIO reads. That allowed VBlank to freeze while the guest waited for the very interrupt it expected.

In 0.0.47, `--device-clock-host` schedules PVR VBlank from the same host-clock source at the configured PVR cadence. The heartbeat reports:

```text
pvr-packets=
pvr-frames=
pvr-vblanks=
pvr-renders=
pvr-flips=
pvr-hostticks=
```

This is a host-synchronized execution aid, not a claim of cycle-accurate Dreamcast video timing.

## 5. Current retail acceptance

The latest diagnostic run remained active through extended diagnostic windows without `DreamcastRecomp ERROR`. It passed the old memory failures, crossed task/context switches and progressed through repeated GD-ROM work. The observed disc path reached at least 14 requests and a real 18-sector transfer.

The next demonstrated boundary is now hardware progress rather than a crash:

```text
PVR VBlank:     advancing
TA packets:     0
PVR frames:     0
PVR renders:    0
```

Therefore the next priority is to determine why the retail Kamui/TA path has not yet submitted a polygon list: PVR register/bootstrap state, Store Queue/TA destination setup, interrupt masks/status, or the next synchronization dependency. The commercial ARM7 blank-reset diagnosis from 0.0.46 also remains open; 0.0.47 still does not apply an implicit 2 MiB RAM wrap.

## Windows acceptance command

```bat
run_commercial_recompiled.bat "RUTA\ChuChu Rocket!.cdi"
```

For the next iteration, the most useful output is the first heartbeat where any of `pvr-packets`, `pvr-frames` or `pvr-renders` becomes non-zero, or the final heartbeat/error if they remain zero.
