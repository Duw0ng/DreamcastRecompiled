# Validation — DreamcastRecomp 0.0.27

Validation performed on the 0.0.27 source tree after the PVR lifecycle and native AICA ARM7 changes.

## Project build

```text
cmake -S . -B build_linux -DCMAKE_BUILD_TYPE=Release
cmake --build build_linux -j4
```

Result: project and all test executables compiled successfully.

## CTest

```text
ctest --test-dir build_linux --output-on-failure -j4

100% tests passed, 0 tests failed out of 47
```

The new `arm7_tests` covers ARM arithmetic/conditions, memory, BL/BX, MRS/MSR mode switching, banked SP state, LDM/STM and explicit unsupported Thumb-state detection.

## Standalone generated native project

`cpp_emitter_tests` generated the literal-pool sample into its own output directory. That generated source was then independently configured and built with CMake.

Generated library sources include:

```text
dc_runtime.cpp
dc_arm7.cpp
dc_image.cpp
dc_native_overrides.cpp
generated_main.cpp
```

`generated_compile_test` passed its ARM/AICA reset smoke test (`MOV r0,#42; B .`).

The generated native runner also executed the SH-4 literal-pool sample successfully:

```text
DreamcastRecomp 0.0.27 native runner
Executing _main @ 0x8C010000u
Hello from DreamcastRecomp literal-pool test!
[DreamcastRecomp] returned to host ... PC=0xFFFFFFFF
```
