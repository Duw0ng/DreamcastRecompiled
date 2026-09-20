# DreamcastRecomp 0.0.192 validation

## Failure-driven changes

- `230404`: dynamic call from `0x8C0360A2` reached valid SH-4 at `0x0C03E700` but no target was registered. Raw-boot seed recompilation proved it is a clean wrapper; supplemental closure now registers it and four other callable holes/dependencies found by audit.
- `230534`: `0x8C075070` formed invalid lookup `0x87B1CA68` after reading `0x6F72746E` (ASCII `ntro`) as a record index from a freshly GD-ROM-loaded buffer. 0.0.192 records the last eight variable-record hops; no recovery/skip is forced yet.

## Static validation

- Supplemental literal closure scan: 9003 callable literal references, 42819 registered targets, 0 unresolved.
- Modified CT2 translation units compile individually: `generated_ct2_rules178.cpp`, `generated_program_part_15.cpp`, `dc_runtime.cpp`.
- Core/codegen test suite: 51/51 pass.

## Performance path

- Direct Type-7/8 staged-strip path is no longer disabled by `--perf-profile` in either the CT2 runtime or the generic emitter.
