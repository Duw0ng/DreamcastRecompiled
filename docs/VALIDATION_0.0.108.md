# Validación 0.0.108 — recompilación comercial de bajo consumo

## Objetivo

Reducir el consumo de CPU/RAM durante la recompilación comercial sin perder la granularidad incremental introducida en 0.0.107.

## Cambios

- Los 32 shards comerciales se conservan para que cambios pequeños no invaliden toda la closure.
- MSVC ya no recibe `/MP8` en el proyecto generado.
- `run_commercial_recompile.bat` construye con `--parallel 1`: un trabajo de compilación a la vez.
- `build_windows.bat` también usa `--parallel 1` por defecto para evitar saturar el equipo durante el build inicial de herramientas.
- Nuevo `tools/build_progress.ps1`: intercepta la salida del build comercial y muestra una barra aproximada por archivo `.cpp`, porcentaje y `N/total`.
- La caché incremental de 0.0.107 se conserva: el directorio `cpp/build` no se borra salvo `DCR_CLEAN_RECOMPILE=1` y el codegen no reescribe archivos cuyo contenido no cambió.
- Se mantienen `/O2 /Oi /Ot /Gy` y LTO/IPO desactivado; no se sacrifica optimización del runner para obtener menor consumo durante el build.

## Validación de closure comercial

Sobre el CDI de validación ChuChu Rocket:

- 2965 funciones registradas.
- 295215 instrucciones SH-4 conocidas.
- 0 unknown / `RAW_SH4=0`.
- 32 `generated_program_part_XX.cpp`.
- El CMake generado no contiene `/MP`.
- Runner generado reporta `DreamcastRecomp 0.0.108`.

## Tests

- 47/47 CTest PASS.
- Runner pequeño recién emitido compila con `--parallel 1`, enlaza y ejecuta correctamente.
- `generated_compile_test` retorna 0.

## Próximo paso

Capturar `run_commercial_recompiled_perf.bat` en la escena pesada que cae a ~25–31 FPS y usar `[DCR PERF]` / `[DCR PERF HOTPC]` para 0.0.109. No se introducen optimizaciones ingame especulativas en 0.0.108.
