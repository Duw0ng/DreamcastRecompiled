# DreamcastRecomp 0.0.202_rebased193 validation

## SH-4 address-taken proof engine

0.0.202 adds a conservative global scan for address-taken SH-4 entry points inspired by the useful closure ideas in dcrecomp, while retaining DreamcastRecomp's stricter CFG validation.

Candidates must be aligned local code pointers that occur repeatedly and are observed inside clustered pointer data. A candidate is promoted to `proven` only if it also has an independent callable-entry shape and `analyze_code_fragment_at()` decodes its reachable CFG with zero unknown SH-4 instructions.

The raw recompiler emits `sh4_address_taken_evidence.csv` with `candidate`/`proven` state, reference counts and decoded instruction/call counts.

### Crazy Taxi 2 real target

On the CT2 bootstrap used for local validation:

- `0x8C03650C` appears 16 times as an aligned address-taken value.
- It is classified `proven`.
- Fragment: 215 instructions / 215 known / 0 unknown / 11 calls.
- It is present in the final function map before runtime execution.

Fresh 0.0.202 closure:

- synthetic entries: 3571
- reachable functions: 3571
- reachable instructions: 300703
- known SH-4: 300703
- unknown SH-4: 0
- RAW_SH4: 0
- address-taken candidates: 1348
- address-taken promotions: bounded to 512 strongest candidates for this pass

The previous 0.0.201 closure had 3548 reachable functions. The new proof engine adds the missing third-map target without causing a large closure jump.

## Commercial max-functions fix

The raw seed closure and authoritative final `ProgramAnalysis` previously shared the same hard `--max-functions` ceiling. If raw discovery filled that ceiling, the final pass was started with no room for CFG fragments and could immediately fail with:

`Se alcanzo el limite de funciones alcanzables (--max-functions)`

0.0.202 separates the budgets. The normal commercial BAT now uses:

- final ProgramAnalysis budget: 16384
- raw closure-entry budget: 12288
- reserved headroom: 4096 functions

The raw CLI also supports `--max-closure-entries=N`. This is a correctness/safety fix rather than simply doubling the old limit: the final CFG pass always has protected space.

## SH-4 ABI audit

Generated runtimes now accept `--sh4-abi-audit`.

In audit mode fast dynamic dispatch is disabled and ordinary indirect-call returns verify callee-saved SH-4 `R8-R14` plus `R15/SP`. Context-switch/non-local returns are deliberately excluded. Violations print caller, target, register, old value and new value. Normal gameplay keeps the feature disabled and pays no audit-path overhead.

The CT2 audit BAT enables this mode.

## CT2 compatibility package

The experimental CT2 package includes the independently generated `0x8C03650C` native fragment and registers it before the existing supplemental callback families. It also retains the 0.0.202 PVR modifier-volume/stencil work already prepared for shadow-volume compatibility.

## Validation

- Core build: PASS.
- CTest: 51/51 passed.
- Fresh CT2 raw analysis: PASS, zero UNKNOWN / zero RAW_SH4.
- `0x8C03650C` evidence and function-map presence: PASS.
- CT2 runtime translation units containing `dc_runtime.cpp` and `generated_sub_8C03650C.cpp`: compiled successfully in the Release build before the environment time-slice stopped later giant generated shards.
