# cpp35b1 — CACHE_ABI_FREEZE

Checkpoint dedicado a impedir que cambios de runtime/plugins invaliden un guest SH-4 ABI1 ya compilado.

## Cambio principal

- `DCRuntime` Guest ABI v1 queda congelado bajo `runtime/guest_abi/v1/`.
- El launcher ya no compara `dc_runtime.hpp` completo. Solo compara la estructura ABI `DCRuntime` mediante un fingerprint canonicalizado.
- Un cache cpp34x/cpp35 válido puede reutilizarse aunque el header mutable del runtime tenga nuevas APIs Plugin V2.
- Cuando el cache es válido, `modularize_generated.py` NO se ejecuta sobre él y su `CMakeLists.txt`/headers guest no se reescriben.
- Runtime y plugins usan un árbol CMake separado (`runtime_build_cpp35b1`) que deposita `DreamcastRuntime`/plugins junto al runner existente.
- `--rebuild-runtime` ya no incluye `dreamcast_program` ni `dcr_game_module` como targets.

## Garantía de prueba

`TEST_CPP35B1_NO_GUEST_REBUILD.bat` usa `--reuse --rebuild-runtime`.

Si existe un Guest ABI1 válido, reutiliza el módulo ya compilado y solo actualiza runtime/plugins. Si el cache fue borrado previamente o realmente es incompatible, el script aborta en lugar de iniciar una recompilación SH-4 inesperada.

## Compatibilidad legacy

Caches ABI1 anteriores conservan su validez si el layout de `DCRuntime` coincide con el ABI congelado. La validación ya no depende de comentarios, APIs nuevas fuera del struct o del contenido completo del header.
