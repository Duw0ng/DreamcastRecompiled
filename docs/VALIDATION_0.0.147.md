# DreamcastRecomp 0.0.147 validation

- Parent: 0.0.146.
- Accepted stable rollback: 0.0.131.
- Scope: exact guest-TU hot leaves for SQ Store32, SQ FMOV64 store, main-RAM FMOV64 load; heartbeat `pvr-ta-reg`.
- Main Release build: PASS.
- CTest: 50/50 PASS.
- Fresh generated runtime build: PASS.
- `generated_compile_test`: RC=0.
- `dreamcast_program`: RC=0 and identifies itself as DreamcastRecomp 0.0.147.
- No FPU arithmetic, TA ordering, QACR/PREF, PVR renderer, AICA/CDDA, GD-ROM, Maple or presentation semantics changed.
