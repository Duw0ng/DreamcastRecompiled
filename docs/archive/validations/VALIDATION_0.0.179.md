# DreamcastRecomp 0.0.179 validation — RAW_SH4 false-positive cleanup

## Scope

0.0.179 fixes the raw commercial analyzer's code/data boundary for dense Dreamcast pointer tables. P0 (`0x0C...`), P1 (`0x8C...`) and P2 (`0xAC...`) aliases of main RAM are recognized as pointer data when they form a sufficiently dense aligned run. Sparse/mixed blocks are deliberately not classified as dense pointer tables.

## Source regressions

- Added `include/dcrecomp/raw_data.hpp` and `src/common/raw_data.cpp`.
- Added `tests/raw_data_tests.cpp` covering P0 aliases, P2 aliases, runtime main-RAM pointers and sparse non-table input.
- Full suite: **51/51 PASS**.

## Crazy Taxi 2 fresh zero-seed closure

Recovered real CT2 boot image:
- bytes: 1,492,884
- base: `0x8C008000`
- entry: `0x8C008300`
- manual seeds: 0

Final closure:
- passes: 24
- reachable functions: **3,449**
- reachable instructions: **287,944**
- known SH-4: **287,944**
- unknown SH-4: **0**
- RAW_SH4: **0**
- CFG blocks: **52,817**
- DCIR ops: 294,868
- rejected dirty fragments: **0**

The prior closure reported 26,193 RAW_SH4 occurrences. The audit showed those represented 3,857 unique addresses concentrated in two data/table regions rather than missing executable ISA coverage.

## Packaged-target regression audit

Complete native target union (generated core + validated CT2 supplemental modules):
- 0.0.178: **41,671** unique targets
- sanitized 0.0.179: **42,130** unique targets
- old targets absent in 0.0.179: **533**
- absent targets inside the two audited false-data regions: **533/533**
- absent targets outside those regions: **0**
- registered targets remaining inside those two false-data regions: **0**

The earlier apparent loss of 145 valid block entries was a comparison artifact that omitted the supplemental registration module; recomputing the full package union shows those blocks remain registered.

## Generated package

- RAW_SH4 markers in packaged generated C++: **0**
- unknown-SH4 markers in packaged generated C++: **0**
- Linux Clang Debug full generated build/link: **PASS**
- `generated_compile_test`: **RC=0**
- Real recovered CT2 CDI smoke: no new missing-target error observed during the host-limited early VMU/disc-loading run.

Normal Windows gameplay testing remains manual via `experimental_ct2_0.0.179/run_crazy_taxi_2.bat`.
