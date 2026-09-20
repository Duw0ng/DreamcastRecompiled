# DreamcastRecomp 0.0.102 — validación

## Objetivo
Hacer que las capturas `cdda_live.wav` y `aica_live.wav` existan durante la ejecución y no dependan de que el runner alcance su ruta de cierre.

## Cadencia
El runtime guarda snapshots cada **15 segundos** usando `steady_ns()` dentro del device clock host. La cadencia es independiente del número de ciclos SH-4 y de los FPS.

## Semántica de snapshot
- No se llama a `aica_host_stream_finish()` durante el guardado periódico.
- Se escribe primero `PATH.tmp` y luego se publica sobre `PATH`.
- En Windows se usa reemplazo de archivo; en otras plataformas `rename()`.
- El cierre normal conserva el flush final existente.
- El writer periódico escribe header WAV + PCM directamente para evitar una copia adicional de toda la captura.

## Telemetría esperada
```text
[AUDIO PROBE] periodic WAV flush #1 | interval=15s | cdda=written/...s | aica=written/...s
```

## Validación
- Build Release del proyecto: PASS.
- 47/47 CTest PASS.
- Runner recién emitido: `dc_runtime.cpp`, programa generado, compile-test y runner enlazan correctamente.
- Prueba sintética de snapshot generó dos RIFF/WAVE válidos de 44.1 kHz, PCM16 estéreo, con tamaño de data correcto.
