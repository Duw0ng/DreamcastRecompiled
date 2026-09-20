# DreamcastRecomp 0.0.204_rebased193 validation

## Goal

Continue the clean 0.0.203 symbol-free commercial closure, remove the observed ChuChu Rocket! dynamic SH-4 boundaries generically, and reach the first real PVR render/page flip without adding title-specific function seeds.

## Boundary 1 — packed BRA selector family

The first missing runtime target was `0x8C10A518`. It is the final member of a packed seven-entry SDK selector family at `0x8C10A500..0x8C10A518`:

- every entry is four bytes: `BRA shared_worker` plus its architectural delay slot;
- all seven branches target local worker `0x8C10A572`;
- the delay slots encode consecutive `MOV #0,R4` through `MOV #6,R4` selectors;
- discovery expands the family only from an already-proven callable member and requires at least three consecutive matching members.

The rule is structural and does not contain a ChuChu address whitelist. The initial 0.0.204 checkpoint after this fix was **4,762 functions / 474,561 known / 0 unknown / RAW_SH4=0**.

## Boundary 2 — literal-fed compact callback

Executing that checkpoint exposed the next real boundary:

```text
No recompiled/native target registered for Dreamcast address 0x8C04F698
source path around 0x8C0186AE
nearest registered entry: 0x8C04F6C0 (+0x28)
```

`0x8C04F698` is a four-instruction leaf callback:

```text
MOV.L @(disp,PC),R2
MOV   #0,R3
RTS
 MOV.B R3,@R2     ; delay slot
```

Its literal pool begins immediately after the return and contains several in-image pointers. The generic dense-pointer guard therefore mistook the target for data even though a PC-relative literal in already-proven executable code flows unchanged to an architectural `JSR/JMP`.

0.0.204 now gives that exact control-flow provenance precedence only when the target also satisfies the strict compact-callable predicate. Arbitrary pointer-like words still fail closed.

The final fresh closure is:

```text
Reachable functions:     4763
Reachable instructions:  474565
Known SH-4:              474565
Unknown SH-4:                 0
RAW_SH4:                      0
Direct literal targets:      12
CFG literal call targets:     6
Packed BRA selectors:         1
```

`0x8C04F698` appears in the fresh function map as `sub_8C04F698,4,4,0,...` and no manual seed was added.

## Core regression

The complete project test suite passes:

```text
53/53 PASS
```

This includes the generated single-function and generated-program build regressions.

## Commercial runtime acceptance

A local diagnostic build generated from the user's `BOOTSTRAP.BIN` and `ChuChu Rocket!.cdi` was run through the same commercial bootstrap path. It passed both previous missing SH-4 targets and performed real GD-ROM traffic.

The first captured PVR-positive heartbeat contained:

```text
gd-req=23
ta-init=13
list-end-irq=24
render-irq=21
isp-start=7
pvr-packets=309
pvr-tri=136
pvr-frames=7
pvr-renders=7
pvr-flips=6
```

The same bounded run later reached:

```text
pvr-frames=509
pvr-renders=509
pvr-flips=505
pvr-packets=9245
pvr-tri=3568
```

No further unresolved SH-4 target appeared before the bounded local execution ended. This establishes the requested 0.0.204 milestone: **ChuChu reaches genuine TA/PVR render and page-flip activity through the recompiled commercial path**.

The local Linux diagnostic used the software PVR path (`pvr-gpu=off`); the normal Windows runner keeps the existing D3D11/DXGI path and remains the user-facing performance test.

## Distribution

The user's CDI, `BOOTSTRAP.BIN`, extracted game files, generated commercial C++ and runtime logs are test inputs/artifacts only and are **not** included in the release ZIP.
