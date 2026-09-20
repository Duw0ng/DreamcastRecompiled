# DreamcastRecomp 0.0.149 validation

- Parent and immediate experimental rollback: 0.0.148.
- Accepted stable rollback: 0.0.131.
- Scope: remove the remaining first-triangle individual emission from complete non-translucent staged strips while preserving final triangle-list indices and state semantics.
- Complete Type-8 regression: four vertices -> exact indices `0,1,2 / 1,2,3`.
- Complete Type-8 opbulk regression: `1 strip / 2 triangles / 6 indices`.
- Incomplete Type-8 regression: zero opbulk runs; existing rolling path remains active.
- Translucent path: unchanged `PVRDeferredTriangle` sort metadata and stable sorting.
- Main Release build: PASS.
- CTest: 50/50 PASS.
- Fresh generated runtime configure/build: PASS.
- `generated_compile_test`: RC=0.
- `dreamcast_program`: RC=0 and banner `DreamcastRecomp 0.0.149 native runner`.
- No FPU arithmetic, QACR/PREF, TA packet order, texture decoding, shader semantics, AICA/CDDA, GD-ROM, Maple or presentation changes.
