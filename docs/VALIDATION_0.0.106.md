# Validación 0.0.106 — CDDA YB exacto

## Hallazgo de 0.0.105

La captura fuente de 0.0.105 confirmó que el ruido residual seguía dentro del CDDA reconstruido. El runtime todavía ejecutaba el de-click después de una interpolación lineal, mientras la vista previa aprobada había usado limpieza antes del remuestreo y un filtro polifásico band-limited.

## Pipeline 0.0.106

1. Detectar sector YB + prefijo SYNC.
2. Descramble Yellow-Book.
3. Extraer 585 frames PCM válidos (2340 bytes).
4. Construir contexto de ±2 sectores.
5. Mediana temporal de 5 muestras, umbral 2048, 4 pasadas, antes del resampling.
6. Remuestreo polifásico 196/195, FIR Kaiser beta=5, 196 fases × 21 taps efectivos.
7. Emitir 588 frames a 44.1 kHz.

## Validación numérica

- Reconstrucción directa desde el CDI: 40 s / 3000 sectores YB.
- Salida de referencia: `scipy.signal.resample_poly(clean, 196, 195)`.
- Implementación sectorial con ventana de 5 sectores: diferencia exacta 0 PCM16 en los primeros 100 sectores comparados contra el pipeline global.
- Pipeline global frente a la vista previa aprobada: correlación L/R 0.999996 / 0.999998; MAE aproximado 4.15 / 2.76 unidades PCM16.
- Suite: 47/47 CTest PASS.
- Runner recién emitido: `dc_runtime.cpp`, `generated_program.cpp`, `generated_compile_test` y `dreamcast_program` compilan/enlazan.

## Baseline

0.0.94 permanece congelada como rollback estable.
