# DreamcastRecomp v1.2 cpp35b1 CACHE_ABI_FREEZE

- Congela formalmente Guest ABI v1 en `runtime/guest_abi/v1`.
- Corrige la falsa invalidación de cache causada por comparar `dc_runtime.hpp` completo.
- Acepta caches cpp34x/cpp35 cuyo `DCRuntime` tenga el mismo layout ABI1.
- Los caches válidos pasan a ser inmutables durante actualizaciones del runtime: no se vuelve a ejecutar el modularizador ni se reconfigura su CMake.
- Separa el build de `DreamcastRuntime`, `dcr_network` y `dcr_maple` del build de `dcr_game_module`.
- `--rebuild-runtime` nunca solicita el target `dreamcast_program`.
- Añade `TEST_CPP35B1_NO_GUEST_REBUILD.bat`, que usa `--reuse` para garantizar que una prueba no dispare silenciosamente una recompilación completa.
- Mantiene Maple cpp35b private-state y Network cpp34s sin cambios funcionales.
