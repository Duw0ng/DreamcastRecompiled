# DreamcastRecomp 0.0.111 — low-memory compile hotfix

## Motivo

0.0.109/0.0.110 movieron los helpers de registros FPU y los fast paths de RAM/Store Queue al `dc_runtime.hpp` como `__forceinline`. En MSVC `/O2`, cada shard comercial veía esos cuerpos y el optimizador los expandía en una gran cantidad de call sites del SH-4 recompilado. En un equipo de 16 GiB esto elevó el conjunto de trabajo del proceso de compilación hasta ~8–10 GiB aun compilando un solo shard a la vez.

## Cambio

- `dc_read32_hot` / `dc_write32_hot` conservan la clasificación rápida de SDRAM/SQ, pero se compilan una sola vez en `dc_runtime.cpp`.
- Los helpers FPU (`dc_get/set_fr_bits`, `dc_get/set_xf_bits`, DR/XD/FMOV64 y checks FPSCR) vuelven a una única implementación out-of-line en `dc_runtime.cpp`.
- Los shards solo reciben declaraciones; se elimina `DCR_MEM_FORCE_INLINE` y `DCR_FPU_FORCE_INLINE` del header generado.
- `<bit>` deja de ser requisito del header runtime; permanece donde realmente se usa, en el runtime source.
- Se conserva la ventana fija PVR de 3 vértices de 0.0.109 y las llamadas generadas a los fast paths de 32 bits.
- Se conserva el build comercial serial de bajo consumo: `--parallel 1`, sin `/MP`, con caché incremental y barra de progreso.

## Objetivo

Recuperar el perfil de memoria de compilación de 0.0.108 sin volver a la ruta genérica completa de memoria y sin cambiar timings, CDDA, AICA, PVR/D3D11 ni la closure SH-4.
