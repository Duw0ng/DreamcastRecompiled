# DreamcastRecomp 0.0.174 — broader SH-4 AOT entry discovery

## Objective

Broaden symbol-free SH-4 coverage before further Crazy Taxi 2 gameplay testing, reducing the need to add one-off missing targets after each dynamic dispatch failure.

## General SH-4 analyzer changes

- Adds finite masked-selector `BRAF` recovery. A pattern such as `AND #(2^n-1),R0` followed only by fixed left shifts and `BRAF R0` is expanded into a bounded set of real CFG targets.
- Adds conservative symbol-free function-entry recovery after proven terminal/literal-pool boundaries using strong SH-4 ABI prologues.
- Adds a strict adjacent `RTS + delay-slot -> FPU ABI prologue` path for substantial FPU-heavy routines.
- Prevents a decodable data halfword immediately before a function from shifting the synthetic entry backwards: a strong ABI prologue may contain at most one setup instruction before the first callee-save stack action.
- Keeps all recovered entries as normal AOT seeds so the generated dispatcher registers them as first-class guest PCs.

## Crazy Taxi 2 static acceptance

Using the same 1,492,884-byte `.raw_boot` image as 0.0.173, with no manual seed entries:

- Synthetic entries: **3,553**
- Reachable functions: **3,573**
- Reachable SH-4 instructions: **335,095**
- Known SH-4 instructions: **308,902**
- Unknown SH-4 instructions: **26,193**
- RAW_SH4: **26,193**
- CFG blocks: **54,507**
- Call-graph edges: **11,013**

The unknown/RAW count does not increase versus the 0.0.173 baseline used during this pass.

The analyzer now recovers the following previously diagnostic targets without title-specific seeds:

- `0x8C06040A` — 70/70 known
- `0x8C060654` — 83/83 known
- `0x8C060728` — 69/69 known
- `0x8C06B4C8` — 116/116 known
- `0x8C081062` — 18/18 known
- `0x8C08106A` — 33/33 known

The earlier false boundary `0x8C060726` is not registered; the actual ABI entry is `0x8C060728`.

## Compatibility carried forward

The packaged `experimental_ct2_0.0.174` keeps the 0.0.173 Crazy Taxi 2 dynamic closure (`0x8C0372A0`, `0x8C037790`, post-title targets, allocator reconstruction), clean manual VMU input, and no investigation-only auto A/YES/START hooks. The new generic generated program is overlaid underneath those already validated CT2 compatibility modules.

## Validation

- Main project test suite: **50/50 PASS**.
- Generic 0.0.174 generated commercial output: Clang Debug full build/link **PASS**.
- Generic generated `generated_compile_test`: **RC=0**.
- Packaged CT2 merged build: Clang Debug full build/link **PASS**.
- Packaged CT2 `generated_compile_test`: **RC=0**.

Windows gameplay acceptance remains the user's next test; this environment does not substitute a Windows/MSVC/XInput/D3D11 playthrough.
