# DreamcastRecomp 0.0.98 - validación

## Objetivo

0.0.98 endurece el CDDA real de 0.0.97 después de una prueba comercial en la que el primer audio fue correcto y una transición posterior produjo estática aun con `cdda-pcm` activo, cero fallos de lectura y cero track misses. No modifica las optimizaciones SH-4/PVR.

## Cambios verificables

- En `--device-clock-host`, el playhead GD-ROM/CDDA usa wall time a 75 sectores/s y `dc_gdrom_tick()` no vuelve a avanzarlo desde ciclos SH-4.
- El loose-lock PCM no salta sectores por diferencias pequeñas de FAD cuando ambos clocks son host-paced; los cambios de rango siguen reanclando.
- Heartbeat `cdda-clock=host75|guest75`.
- Detección de byte-order por pista mediante energía de diferencias entre muestras consecutivas L/R.
- LE continúa siendo el default de DiscJuggler; BE requiere una ventaja de continuidad de al menos 20%.
- Sectores silenciosos no cuentan para decidir el orden.
- Tras 16 sectores audibles ambiguos se conserva LE y el heartbeat lo marca como `le?`.
- `cdda-src=Tn/FAD/FRAME/ORDER/ANALYZED/LE_DELTA/BE_DELTA` identifica la pista y la decisión actual.
- `--cdda-wav=PATH` captura PCM CDDA antes de AICA/MVOL/WinMM.
- El audio probe produce `cdda_live.wav` y `aica_live.wav` y mantiene el raster D3D11 GPU.
- `dc_disc_probe` muestra la tabla completa de pistas del CDI.

## Validación host

- Configuración y build Release: PASS.
- CTest: 47/47 PASS.
- Proyecto C++20 recién generado: compila y enlaza.
- Test de reloj host sintético: 1.000 s -> 75 ticks / FAD +75; añadir después 200.000.000 ciclos SH-4 no produce doble avance.
- Test PCM sintético:
  - seno estéreo PCM16 LE -> detector selecciona `le` en un sector;
  - mismo PCM con bytes invertidos -> detector selecciona `be` en un sector.

## Validación comercial pendiente

Ejecutar `run_commercial_recompiled_audio_probe.bat` con el CDI original, reproducir la misma transición que generó estática en 0.0.97 y conservar el heartbeat `cdda-src`. Si persiste el ruido, comparar `generated/commercial_recompiled/cdda_live.wav` con `aica_live.wav`: si el primero ya contiene estática, el problema está en selección/layout/decodificación de la pista; si el primero es correcto y el segundo no, el problema queda después del CDDA source decoder.
