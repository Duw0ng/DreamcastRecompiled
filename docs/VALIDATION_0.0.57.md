# DreamcastRecomp 0.0.57 validation

- Main CMake/CTest regression: **47/47 PASS**.
- ChuChu commercial closure: **2,294 reachable functions**.
- Reachable SH-4 instructions: **171,568 known / 0 unknown**.
- Generated DCIR: **`RAW_SH4=0`**.
- The six-handler overlapping state family is present in the generated map, including the runtime-observed `0x8C05D55A`.
- The complete 2,294-function commercial `generated_program.cpp` compiles with Clang 17 at `-O0`, and the generated runtime/runner links successfully on the Linux validation host.
- No runtime hardware/performance changes relative to 0.0.56.
