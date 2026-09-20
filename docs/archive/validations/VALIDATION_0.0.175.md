# DreamcastRecomp 0.0.175 — CT2 vtable closure + no-regression packaging

## Objective

Fix the 0.0.174 Crazy Taxi 2 regression that stopped before `NOW LOADING` at dynamic target `0x0C14AD06`, while keeping the broader SH-4 analyzer work and preventing future releases from dropping targets that were already present in 0.0.173.

## Root cause

The failing call site at `0x8C14B43C` loads a method from `[object + 0x74]` and jumps to it. The selected value is `0x0C14AD06` (canonical P1 `0x8C14AD06`). The raw image contains that pointer inside a 25-entry dense callback/vtable run around `0x8C16FE40`, but 0.0.174's generic regeneration did not rediscover the whole table and the packaged merge dropped 64 targets that had existed in 0.0.173.

`0x8C14AD06` itself is clean SH-4: 26/26 decoded instructions, 0 unknown, terminating in `RTS` with its literal pool after the delay slot.

## Analyzer change

- Adds an anchored dense callback-table discovery pass.
- A candidate table must contain at least eight consecutive local code pointers.
- At least four members must already be proven by the current closure and the anchored density must pass a conservative confidence threshold.
- Missing members are promoted only when they pass callable/CFG validation.
- P0/P1/P2 code-pointer aliases are canonicalized before validation.

On the same Crazy Taxi 2 `.raw_boot` image, the updated analyzer reports:

- Synthetic entries: **3,678**
- Reachable functions: **3,698**
- Reachable instructions: **343,867**
- Known SH-4: **317,674**
- Unknown SH-4: **26,193**
- RAW_SH4: **26,193**
- Anchored dense callbacks promoted: **231**

The RAW/unknown count does not increase versus 0.0.174.

## Runtime package strategy

To minimize runtime risk for the immediate Crazy Taxi 2 test, the packaged `experimental_ct2_0.0.175` keeps the already-built 0.0.174 generated program and adds only a small AOT supplement containing the nine missing vtable roots required to restore the 64 targets lost from 0.0.173. Existing 0.0.174 mappings are not overwritten unnecessarily.

Representative restored roots include:

- `0x8C14AB06`
- `0x8C14AB9E`
- `0x8C14AC76`
- `0x8C14AD06`
- `0x8C14AD48`
- `0x8C14AFF6`
- `0x8C14B0B2`
- `0x8C14B1D2`
- `0x8C14B268`

## No-regression target audit

- 0.0.173 packaged targets: **36,018**
- 0.0.175 packaged targets: **41,141**
- Targets lost relative to 0.0.173: **0**
- Additional targets relative to 0.0.173: **5,123**

The generated compile test explicitly checks all 64 targets accidentally dropped by 0.0.174, including `0x8C14AD06`.

## Validation

- Main project: **50/50 CTest PASS**.
- Packaged CT2 Debug/Ninja full build and link: **PASS**.
- Packaged `generated_compile_test`: **RC=0**.
- Static no-regression target audit versus packaged 0.0.173: **PASS (lost=0)**.

Windows gameplay acceptance is still required. The expected first result is restoration of the 0.0.173 boot path past the pre-`NOW LOADING` failure; if a later missing target appears, its log should be used to continue the generic callback/block-entry work.
