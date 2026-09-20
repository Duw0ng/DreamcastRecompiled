# DreamcastRecomp 0.0.100 — validación

## Objetivo

Corregir la única música CDDA todavía defectuosa sin tocar rendimiento, PVR, Maple, ARM7/AICA ni el reloj host75.

## Hallazgo sobre el CDI real

La pista lógica T4 no usa una sola representación física:

- FAD 45308..49601: 4294 sectores Yellow-Book scrambled.
- FAD 49602..53385: 3784 sectores PCM raw.
- Hay exactamente una transición y ningún sector ambiguo con el umbral 4x.

Los 4294 sectores Yellow-Book comienzan con el marcador de 12 bytes `00 FF FF FF FF FF FF FF FF FF FF 00`. Ese marcador no es PCM. Después de descramblar bytes 12..2351, el primer frame PCM útil (frame 3) mantiene continuidad con el último frame del sector anterior.

## Corrección

- Selección RAW/YB por sector, no por pista.
- Sectores ambiguos heredan el modo anterior; el primer ambiguo usa RAW.
- Conteo de transiciones internas `cdda_audio_scramble_transitions`.
- En sectores YB con SYNC exacto, los frames 0..2 se interpolan entre el frame final anterior y el frame 3 actual.
- Conteo `cdda_audio_sync_repairs`.
- El análisis LE/BE sigue ejecutándose después del descrambling y antes de la reparación del prefijo.

## Telemetría

`cdda-src=Tn/FAD/FRAME/ENC/ENC_N/RAW_DELTA/YB_DELTA/xTRANS/rREPAIRS/ORDER/ORDER_N/LE_DELTA/BE_DELTA`

Para T4 se espera `yb` antes de FAD 49602 y `raw` después; `xTRANS` debe pasar de 0 a 1 una sola vez.

## Validación host

- CMake Release: PASS.
- CTest: 47/47 PASS.
- Runner pequeño recién generado: compilación y enlace completos PASS.
- Barrido directo de los 8078 sectores T4: 4294 YB + 3784 RAW, transición única en FAD 49602.
- Los 4294 sectores YB tienen el marcador SYNC.
- La reparación reduce la discontinuidad media entre sectores de ~1450 a ~300–337 unidades de sample.

La validación audible final debe realizarse en Windows con `run_commercial_recompiled_audio_probe.bat` y el CDI externo del usuario.
