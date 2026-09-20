# DreamcastRecomp 0.0.94 - validacion

## Objetivo

Eliminar el readback GPU->CPU y la presentacion GDI del camino normal de frames D3D11, conservando readback bajo demanda para RTT/dumps/fallback y manteniendo el renderer software MT como fallback.

## Validacion realizada

- CMake Release Linux: PASS.
- CTest: 47/47 PASS.
- ChuChu Rocket retail closure: 2965 funciones, 295215 instrucciones SH-4 conocidas, 0 unknown, RAW_SH4=0.
- `dc_runtime.cpp`, `dc_arm7.cpp`, `dc_image.cpp`, `dc_native_overrides.cpp`, `generated_runner.cpp`: Clang C++20 syntax PASS en ruta no-Windows.
- `generated_program.cpp` comercial (~55 MiB): Clang C++20 syntax PASS.
- CMake generado enlaza `d3d11`, `d3dcompiler` y `dxgi`.

## A/B Windows

Normal: `run_commercial_recompiled.bat` -> `--pvr-gpu`, direct DXGI present.

Control: `run_commercial_recompiled_gpu_readback.bat` -> `--pvr-gpu-readback`, fuerza readback + GDI.

Software: `run_commercial_recompiled_software.bat` -> PVR MT sin D3D11.

Campos a comparar: `fps=`, `pvr-gpu=`, `pvr-gpresent=`, `pvr-gtex=`, `pvr-mt=`.
