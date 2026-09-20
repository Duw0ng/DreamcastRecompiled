# DreamcastRecomp 0.0.103 — validación

## Objetivo
Eliminar la estática residual observada en la parte Yellow-Book de CDDA sin tocar pistas RAW que ya reproducen correctamente.

## Evidencia desde las capturas reales
Se analizaron las capturas periódicas producidas por 0.0.102:

- `cdda_live.wav`: 44.1 kHz, PCM16 estéreo, 36.696 s.
- `aica_live.wav`: 44.1 kHz, PCM16 estéreo, 44.848 s.
- Outliers respecto de una mediana temporal de cinco muestras, usando umbral 8192:
  - CDDA: 3230 izquierda / 668 derecha.
  - mezcla AICA: 3283 izquierda / 682 derecha.

La casi igualdad demuestra que el artefacto ya existe en el PCM CDDA antes del mixer AICA/WinMM.

## Corrección
- El de-click continúa habilitado exclusivamente para sectores `yb`.
- Cada canal usa una mediana temporal de cinco muestras.
- Solo se reemplaza una muestra si su distancia a la mediana supera 8192 unidades PCM16.
- Se realizan dos pasadas para cubrir ráfagas cortas de dos o tres muestras.
- Sectores `raw` permanecen bit-identical respecto de 0.0.102.
- El contador existente `dN` continúa reportando todas las muestras reparadas.

## Validación sobre la captura del usuario
Aplicando exactamente el nuevo algoritmo a `cdda_live.wav`:

- pasada 1: 3898 reparaciones;
- pasada 2: 231 reparaciones;
- total: 4129;
- outliers residuales >8192: 3230/668 -> 3/0.

En el tramo RAW identificado como música correcta, el mismo umbral detectó cero muestras a modificar.

## Build y regresiones
- Build Release del proyecto: PASS.
- 47/47 CTest PASS.
- Runner recién emitido: `dc_runtime.cpp`, programa generado, compile-test y runner enlazan correctamente.
