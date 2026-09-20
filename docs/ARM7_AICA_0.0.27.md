# Native AICA ARM7 bootstrap — DreamcastRecomp 0.0.27

0.0.27 introduces the first non-HLE AICA CPU path.

## Core

`include/dcrecomp/arm7.hpp` and `src/aica/arm7.cpp` implement the initial ARM7TDMI ARM-state interpreter. It models condition execution, the barrel shifter, common data-processing instructions, branches, multiply, memory transfers, block transfers, CPSR/SPSR and banked registers.

Dedicated unit coverage exercises:

- arithmetic and conditional branches;
- STR/LDR little-endian memory;
- BL/BX return flow;
- the `MRS -> BIC/ORR -> MSR` mode-switch pattern used by ARM firmware startup;
- banked SVC/IRQ stack pointers;
- STMIA/LDMIA;
- explicit detection of a transition into currently unsupported Thumb state.

## Generated runtime

The C++ emitter embeds the ARM7 source into generated projects as:

```text
dc_arm7.hpp
dc_arm7.cpp
```

`dc_runtime.cpp` contains `AICAARM7Bus`, which exposes:

```text
ARM 0x00000000..0x001FFFFF  AICA RAM
ARM 0x00800000..0x0080FFFF  AICA registers
```

SH-4 writes to the AICA reset control register are observed. When bit 0 of SNDREG `0x2c00` transitions to released state, the ARM context resets to PC 0 and begins executing.

The first scheduler is intentionally deterministic and simple: SH-4 accesses to shared AICA RAM/registers allow an ARM instruction slice. Use:

```text
--aica-arm7
--aica-arm7-slice=N
```

The default slice is 128 instructions. Reset release grants a larger bootstrap slice so startup code can establish its stacks and shared structures.

## Standalone generated-project validation

`generated_compile_test` writes this ARM program into AICA RAM:

```asm
mov r0, #42
b .
```

It then releases AICA ARM reset through the SH-4 register aperture and verifies that the generated ARM core reaches `R0 == 42`. This ensures the emitted project contains a functional CPU + bus + reset connection, not merely source files that compile.

## Still missing

This is an execution bootstrap, not yet a complete AICA emulator. Pending work includes ARM exception entry/FIQ/IRQ, timers/interrupt registers, possible Thumb execution, accurate channel/DSP behavior, PCM mixing, ADPCM and better concurrent scheduling.
