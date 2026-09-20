# Validación 0.0.163

## Objetivo

Recuperar el headroom de rendimiento observado en 0.0.151-0.0.156 sin perder las correcciones funcionales acumuladas hasta 0.0.160. La decisión principal es sacar el cache GPR function-wide de la ruta de producción y conservarlo solo para A/B.

## Evidencia que motivó el cambio

- En un punto comparable de ChuChu Rocket! (~353 frames, guest PC 0x8C0FE5A0), 0.0.156 registró ~81 FPS mientras una ejecución 0.0.159 rondó ~57 FPS: una diferencia aproximada de 5.20 ms por frame.
- El BAT CPU-safe anterior no era una comparación limpia porque desactivaba/omitía varias opciones rápidas además del GPR cache. 0.0.163 corrige el launcher para conservar las mismas opciones del runner normal.
- Un PERF 0.0.159 terminó con 65.831 s de wall time y 2,566 frames. El perfil estimó 34.116 s en full tick, 25.504 s en device, 19.168 s en PVR clock y 6.336 s en host sync. Esas categorías se solapan, pero señalan el scheduler/device clock como el siguiente objetivo después de recuperar el camino CPU.

## Política de codegen 0.0.163

- Default: `DCR_GPR_CACHE=0` (context-backed GPRs).
- Experimento: `DCR_GPR_CACHE=1` conserva el cache function-wide endurecido de 0.0.159/160.
- Hot-trace production codegen permanece OFF.
- FPU superblocks, SQ→TA zero-copy, PVR GPU/MT/direct present, fast dispatch, direct dispatch y SH-4 tick batch=256 permanecen activos como antes.

## Launchers

- `run_commercial_recompiled.bat`: ruta recomendada 0.0.163.
- `run_commercial_recompiled_perf.bat`: misma política GPR OFF + profiler.
- `run_commercial_recompiled_cpu_safe.bat`: ahora es un A/B válido con las mismas opciones rápidas del normal.
- `run_commercial_recompiled_gpr_experiment.bat`: reactiva GPR function-wide.
- `run_commercial_recompiled_156_baseline.bat`: GPR OFF + hot-trace codegen/runtime ON para reproducir la forma 0.0.156.

## Validación realizada

- CMake Release host build: PASS.
- CTest: 50/50 PASS.
- Generated C++ project configure/build: PASS.
- `generated_compile_test`: PASS.
- `dreamcast_hello`: PASS; retorna a host con `PC=0xFFFFFFFF`.
- Generated default source audit: no contiene `gpc_flush`, `gpc_reload` ni `gpc_tick`; sí usa `dc_runtime_tick_fast`.

## Límite

No se incluye ni se ejecuta aquí una imagen comercial de ChuChu Rocket!, por lo que la recuperación exacta a 70-80 FPS requiere el test Windows del usuario. El primer checkpoint recomendado es el mismo tramo temprano donde 0.0.156 alcanzaba ~79-81 FPS.
