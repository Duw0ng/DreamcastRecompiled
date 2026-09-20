# DreamcastRecomp 0.0.92 validation

## Objetivo

Medir el coste real del raster PVR usando una ruta D3D11 experimental sin modificar SH-4, AICA, TA ni la sincronizacion que ya funciona en 0.0.91.

## GPU

```bat
run_commercial_recompiled.bat "RUTA\ChuChu Rocket!.cdi"
```

Esperado en heartbeat:

```text
pvr-gpu=on/ready/FRAMES/FALLBACKS/DRAWS/TOTAL_MS/READBACK_MS
pvr-gtex=UPLOADS/REUSES/UNSUPPORTED
```

- `FRAMES` debe crecer continuamente.
- `FALLBACKS` idealmente debe permanecer en 0 o muy bajo.
- `UNSUPPORTED` debe permanecer en 0 durante escenas cubiertas.
- `pvr-mt` no deberia sumar frames mientras GPU complete la escena; sigue habilitado solo como fallback.
- `TOTAL_MS` incluye draw + sincronizacion/readback. `READBACK_MS` permite saber cuanto del coste se debe al puente GPU->CPU temporal de 0.0.92.

## Software A/B

```bat
run_commercial_recompiled_software.bat "RUTA\ChuChu Rocket!.cdi"
```

Esperado:

```text
pvr-gpu=off/wait/0/0/0/0/0
pvr-mt=on/WORKERS/FRAMES/MS
```

Comparar una misma escena (menu con menu o gameplay con gameplay), no mezclar title-screen con partida. Revisar `fps=`, estabilidad visual, `pvr-gpu`, `pvr-gtex`, `pvr-mt` y que `audio-starves` siga cercano a cero.

## Criterio para la siguiente version

1. Si GPU da paridad visual y una mejora clara aun con readback, quitar el readback/GDI y presentar directamente desde GPU sera el siguiente paso.
2. Si GPU queda cerca del software pero `READBACK_MS` explica la diferencia, pasar a swapchain/direct-present antes de concluir que el CPU sigue siendo el cuello principal.
3. Si GPU es rapido pero `fps` apenas cambia, entonces el cuello restante esta principalmente fuera del raster y volvemos al profiler SH-4/runtime.
4. Si `FALLBACKS` o `UNSUPPORTED` crecen, primero completar cobertura PVR; esos FPS no sirven para comparar GPU vs CPU.

## Validacion hecha en contenedor

- 47/47 CTest PASS.
- ChuChu: 2965 funciones, 295215 instrucciones conocidas, 0 unknown, RAW_SH4=0.
- `generated_program.cpp`, `dc_runtime.cpp`, `dc_arm7.cpp`, `dc_image.cpp`, `dc_native_overrides.cpp` y `generated_runner.cpp` pasan syntax C++20 en Linux.
- No hay Windows SDK/D3D11 disponible en el contenedor; el bloque `_WIN32` se compila por primera vez con el MSVC del equipo de prueba.
