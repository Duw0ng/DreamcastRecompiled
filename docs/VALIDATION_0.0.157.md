# Validation — 0.0.157

Required before packaging:

- Host CTest suite: 50/50 PASS.
- Generated C++ project: clean configure + Release build PASS.
- `generated_compile_test`: RC=0, including TA staging, SQ fusion and `sq-zero` increment.
- GPR cache compile-time A/B: generated sample with cache ON and `DCR_GPR_CACHE=0` must return identical architectural final state.
- System-control sample used to cover SR/GBR paths in addition to ordinary integer arithmetic.
- ZIP must not contain Linux build trees, logs, object files or temporary A/B output.

## Final A/B results

- `sh4_cpu_batch`: cache ON and OFF both returned `R0=42 | FR0=0 | SR=0x1 | GBR=0x0 | PC=0xFFFFFFFF`.
- `sh4_system_control`: cache ON and OFF both returned `R0=42 | FR0=0 | SR=0x0 | GBR=0x2A | PC=0xFFFFFFFF`.
- Fresh 0.0.157 host build: 50/50 CTests PASS.
- Fresh generated 0.0.157 project: Release build PASS; `generated_compile_test` RC=0.
