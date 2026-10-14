# cpp32a — CPP23 callback-closure rebase

## Base

`DreamcastRecomp_v0.1.1_cpp23_GAME_PROFILES_REGRESSION_FIX` from the accepted ChuChu ~60 FPS stable package.

## Why this rebase exists

Two current ChuChu logs stop at the same indirect dispatcher (`0x8C0190A4`) with unregistered native targets:

- `0x8C04FFC0`
- `0x8C034FA0`

The discarded cpp32a was built from an older main snapshot and therefore also reverted cpp23/runtime work. This version ports only the conservative callback-closure rule onto cpp23.

## Static closure rule

It recognizes two-level Katana state tables of the form:

```text
state_table -> row0, row1, row2, ...
rowN        -> method0, method1, method2, metadata...
```

A row must contain three local callable SH-4 method pointers. A row is independently anchored only when at least two methods are already in the closure. At most two strict rows may be bridged between independently anchored rows. Exact duplicate row pointers may extend an already accepted row but cannot bootstrap a family. Every new target is passed through `analyze_code_fragment_at`; unknown instructions reject it.

No ChuChu-specific address whitelist or manual seed is introduced.

## Preserved from cpp23

- per-game profiles and isolated workspaces
- current boot-stage policy
- PVR/TA/D3D11 path
- AICA/CDDA/ARM7 path
- GD-ROM, Maple and VMU
- dispatch/runtime behavior
- existing dense, stored, compact, leaf-GPR and tail-veneer callback detectors

## Runtime check

For ChuChu, rebuild the game from clean once and confirm `Nested state callbacks` stays bounded, both missing targets disappear from the affected menu paths, `RAW_SH4=0`, and normal gameplay remains near the cpp23 stable performance.
