# DreamcastRecomp 0.0.97 - validación

## Alcance

0.0.97 parte de 0.0.96 y congela los cambios de rendimiento. El objetivo es convertir el estado CDDA ya existente en audio real desde el CDI sin alterar PVR/D3D11, SH-4 fast paths, AICA/ARM7, Maple ni la closure comercial.

## Implementación CDDA

- `PLAY_TRACKS (0x14)` interpreta `p0/p1` como números de pista y resuelve su rango FAD.
- `PLAY_SECTORS (0x15)` interpreta `p0/p1` como FADs de 24 bits.
- El parser CDI asigna números de pista globales a través de sesiones; mapas V2 antiguos se normalizan en el runtime si contienen números repetidos/no monotónicos.
- Las pistas `mode=0` con sector físico raw de 2352 bytes alimentan un cache de un sector.
- Cada sector produce 588 frames estéreo PCM16. `dc_gdrom_mix_cdda_frame()` aporta un frame a cada iteración del mixer nativo de 44.1 kHz.
- El lifecycle de drive/repeat sigue bajo `dc_gdrom_tick()` a 75 sectores/s. El cursor PCM se mantiene ligado al FAD del drive en seeks/repeats sin consumir dos veces el contador de loops.
- `GETTOC/GETTOC2` publica las pistas reales del mapa y `REQ_STAT` devuelve la pista del FAD actual.
- `cdda-pcm=raw|none/reads/frames/nonzero/read-fails/track-misses` permite diagnosticar la ruta completa.

## Validación host

- Build Release del proyecto host: PASS.
- CTest: **47/47 PASS**.
- Runner generado desde `samples/sh4_literal_pool.elf`: CMake/compilación C++20 PASS.
- Test CDDA sintético: imagen con una pista raw de 2352 bytes + una pista de datos, mapa legacy con `track 1` repetido entre sesiones. Resultado: normalización a tracks globales 1,2; un sector leído; 588 frames mezclados; 588 frames no-cero; muestras L/R iniciales exactas. PASS.

## Validación comercial requerida

El paquete no incluye ni ejecuta el CDI comercial del usuario. En Windows regenerar la salida comercial con 0.0.97 y probar `Options -> Sound Test`: SFX debe continuar funcionando y Music debe hacer crecer `cdda-pcm` con `raw` y contadores no-cero. Conservar el heartbeat/log de esa prueba para cualquier ajuste de TOC, rango o mezcla antes de volver al trabajo de rendimiento.
