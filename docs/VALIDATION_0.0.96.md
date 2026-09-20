# DreamcastRecomp 0.0.96 - validación

## Alcance

0.0.96 parte directamente de 0.0.95 y cambia solo el hot path de `DynamicCall` generado. PVR, D3D11/DXGI, AICA/ARM7, Maple, GD-ROM, timing guest y closure comercial quedan sin cambios.

## Implementación

- `dc_call_dynamic_fast()` vive en `dc_runtime.hpp` y usa `__forceinline`/`always_inline`.
- Reutiliza el mismo `dispatch_cache[4096]` de 0.0.95.
- En hit no-relocado ejecuta la función registrada directamente desde el TU recompilado.
- Mantiene el historial exacto de 32 CALL mediante `dc_trace_record_inline()`.
- En miss o cuando trace/probes/relocación lo requieren, cae a `call_recompiled()`.
- Telemetría: `dispatch-inline=hits/fallbacks`.

## Prueba objetivo en Windows

Usar el runner comercial normal y repetir una escena estable y una oleada con muchos ratones. Comparar contra 0.0.95: `fps=`, `dispatch-fast=`, `dispatch-inline=`, `sh4tick=`, `pvr-gpu=`, `pvr-gpresent=`, `audio-starves=`.

## Validación host

- Build Release del proyecto host: PASS.
- CTest: **47/47 PASS**.
- `cpp_emitter_tests` verifica que `DynamicCall` emita `dc_call_dynamic_fast()` y que la telemetría `dispatch-inline` exista.
