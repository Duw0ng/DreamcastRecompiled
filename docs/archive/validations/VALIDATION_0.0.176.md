# DreamcastRecomp 0.0.176 — CT2 compatibility checkpoint

## Fixes carried into the actual runner

- `generated_runner.cpp` registers `generated_vtable_175` in the same runtime used by `dreamcast_program`.
- P0/P1 alias target `0x0C14AD06` / `0x8C14AD06` is present in the generated target set.
- New CT2 block-entry `0x8C0383B2` is emitted as AOT code; its probe analysis was 80/80 known SH-4 instructions, 0 unknown.
- The normal CT2 launcher remains manual; automatic A/START injection is opt-in only.

## Local validation

- Clang build of `dreamcast_program`: PASS.
- Clang build of `generated_compile_test`: PASS.
- `generated_compile_test`: RC=0.
- Real supplied CT2 CDI was used during development to reproduce and pass the old `0x0C14AD06` failure and to discover `0x8C0383B2` after START.
- Subsequent automated progression ran beyond those points without another missing-target during the tested window. Visual checkpoint automation remains experimental and is not claimed to prove gameplay yet.

## Testing

Copy your own `ct2.cdi` into `experimental_ct2_0.0.176`.
Run `build_windows.bat`, then `run_crazy_taxi_2.bat` for the real manual test.
Use `run_crazy_taxi_2_audit.bat` only for the optional automatic probe.
