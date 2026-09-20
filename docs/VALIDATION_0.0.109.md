# DreamcastRecomp 0.0.109 — validación

## Motivo

Perfil real de ChuChu Rocket en una escena pesada de 0.0.108:

- FPS observados: 28.0, 28.0 y 25.9.
- Store Queue: ~3.38–3.41 millones de escrituras/s.
- TA/PVR: ~453 mil packets/s y ~278 mil triángulos/s.
- FPU aproximado: ~424 mil FTRV/s, ~421 mil FIPR/s y ~824 mil FMAC/s.
- La función `sub_8C0F9CCC`, activa en los heartbeat calientes, mezcla FPU vectorial, FMOV y Store Queue.

## Cambios

1. Fast path force-inline para read/write 32-bit de main SDRAM y Store Queue.
2. Helpers FR/XF/DR/XD/FMOV64 force-inline dentro de los translation units generados.
3. Triangle strip PVR con ventana fija de tres vértices.
4. Fallback genérico intacto para mappings no cubiertos.
5. Build serial/incremental de 0.0.108 intacto.

## Validación automática

- CTest: 47/47 PASS.
- Closure comercial: 2965 funciones; 295215 instrucciones conocidas; 0 unknown; RAW_SH4=0; 32 shards.
- `sub_8C0F9CCC` generado usa `dc_read32_hot` / `dc_write32_hot`; helpers FPU están en `dc_runtime.hpp` con force-inline.
- Equivalencia de memoria comprobada para aliases de SDRAM 0x0C/0x8C/0xAC/0x0D/0x0E/0x0F y ambas Store Queues; bytes y `sq_writes` coinciden con la ruta genérica.
- Runner fresco pequeño: compila, enlaza y `generated_compile_test` retorna 0.
- Shard comercial que contiene `sub_8C0F9CCC`: compilación independiente PASS.

## Criterio de prueba live

Comparar con la misma escena que en 0.0.108 produjo ~25.9–28 FPS. Revisar FPS mínimo/sostenido y heartbeats. Si mejora, continuar perfilando PREF/SQ commit, PVR TA feed y overhead de diagnóstico; si no mejora, usar `[DCR PERF]` / `[DCR PERF HOTPC]` para elegir el siguiente cuello.
