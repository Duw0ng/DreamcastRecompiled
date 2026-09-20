# DreamcastRecomp 0.0.90 validation

## Baseline

0.0.90 se construye desde 0.0.87. No incorpora el experimento 0.0.88. La intencion es obtener una medicion util del coste restante antes de la siguiente optimizacion y restaurar salida de audio host para el camino comercial.

## Performance profiler

`--perf-profile` es opt-in. El muestreo por defecto usa stride 64: solo 1 de cada 64 `dc_runtime_tick_full` consulta `steady_clock` y actualiza el histograma de PC. Esto evita convertir al profiler en el cuello de botella.

Salida final:

```text
[DCR PERF] wall-ms=... | stride=64 | samples=... | tick-est-ms=... | ui-est-ms=... | tmu-gd-est-ms=... | device-est-ms=... | irq-est-ms=... | pvr-mt-ms=...
[DCR PERF HOTPC] 0x8C........=... ...
```

El objetivo de 0.0.90 no es subir FPS por si mismo sino identificar si la proxima mejora debe atacar fastmem, dispatcher/loops calientes, scheduler o AICA.

## Audio comercial

Los heartbeats previos mostraban `aica-frames` y, en ejecuciones largas, frames no nulos del mixer nativo, pero el runner comercial no pasaba `--aica-play`. Por lo tanto el PCM se producia internamente sin enviarse a WinMM.

0.0.90 agrega `--aica-play` al runner normal, single-thread, CPU-safe y PVR profile. El heartbeat reporta:

```text
aica-nz=...
aica-slots=...
aica-fmt=0x...
audio-play=on
```

`run_commercial_recompiled_audio_probe.bat` agrega una captura WAV. Si `aica-nz > 0` y el WAV contiene audio pero el host sigue mudo, el problema queda localizado en WinMM/dispositivo. Si `aica-nz == 0` durante una escena que deberia sonar, el fallo esta antes del backend host.

## Acceptance test en Windows

1. `run_commercial_recompiled.bat` debe compilar con tiempos equivalentes a 0.0.87 y arrancar ChuChu Rocket.
2. Debe aparecer `[AICA WinMM] live playback enabled (44.1 kHz stereo)`.
3. En gameplay revisar `audio-play=on`, `aica-nz`, `aica-slots`, `fps`, `pvr-mt`.
4. Ejecutar `run_commercial_recompiled_perf.bat`, jugar 20-30 s y cerrar la ventana; compartir `[DCR PERF]` y `[DCR PERF HOTPC]`.
5. Si no hay sonido, usar `run_commercial_recompiled_audio_probe.bat` y comprobar `aica_live.wav`.
