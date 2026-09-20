# DreamcastRecomp 0.0.110 — validación

## Motivo

0.0.109 falló al compilar en Visual Studio/MSVC porque los helpers FPU inline de `dc_runtime.hpp` usan `std::bit_cast`, pero el header no incluía `<bit>` por sí mismo. Cada shard incluía `<bit>` después de `generated_program.hpp`, demasiado tarde porque `generated_program.hpp` ya había incluido `dc_runtime.hpp`.

## Corrección

- `dc_runtime.hpp` generado incluye ahora `<bit>` directamente.
- Se mantiene C++20 en el CMake generado.
- No se cambian las optimizaciones hot-path de 0.0.109 ni la compilación serial/incremental de 0.0.108.
- Test de regresión: `cpp_emitter_tests` exige `#include <bit>` en `runtime_header`.

## Validación host

- 47/47 CTest PASS.
- Runner fresco generado desde `sh4_literal_pool.elf`.
- Header fresco verificado: `<bit>` aparece antes de cualquier helper FPU inline.
- CMake Release del runner fresco compila y enlaza con `--parallel 1`.
- `generated_compile_test` retorna 0.
