# Dreamcast Recompiled 0.0.210 - Lodoss PRELIM 2026-09-19

Checkpoint preliminar para continuar el desarrollo de **Record of Lodoss War**.
No se considera una release estable/general todavía.

## Qué queda integrado

- Handoff comercial de Lodoss con entry `0x8C010000` y stack `R15=0x8C00F400`.
- Inyección START de bajo nivel en respuestas Maple `GETCOND`, activada por `--probe-controller-start-burst`.
- Pulsos START de validación en GETCOND: 8, 20, 40, 80, 140 y 220.
- Detección genérica de tablas compactas de callbacks de 12 bytes.
- Dataflow ampliada/conservadora para callbacks almacenados en estructuras.
- Detección genérica de descriptores de callbacks de 32 bytes.
- Detección genérica de tablas método/estado de 36 bytes (corrige la clase que contenía `0x8C04B19C`).
- Presupuesto de closure específico de Lodoss aumentado de 8192 a 9000 entradas.

## Targets dinámicos superados durante esta línea

- `0x8C02B37C`
- `0x8C02B580` y familia de callbacks compactos relacionados
- `0x8C03D79A`
- `0x8C0AEE46`
- `0x8C04B19C`

Ninguno de los últimos targets se conserva como seed manual: se descubren mediante el análisis genérico.

## Closure validada

- Funciones registradas: **8403**
- Instrucciones SH-4 alcanzables: **678999**
- SH-4 conocidas: **678999**
- SH-4 desconocidas: **0**
- `RAW_SH4`: **0**
- Stored callback targets: **453**
- Strided callback targets: **111**
- `0x8C04B19C`: registrada como `sub_8C04B19C`, 15/15 instrucciones conocidas.

## Smoke test

Runner Linux de validación compilado y enlazado al 100%.
Prueba headless con el CDI de Record of Lodoss War:

- 6/6 pulsos START confirmados.
- 208 frames alcanzados en el último heartbeat de la ventana.
- 640 solicitudes GD-ROM registradas.
- Sin `No recompiled/native target`.
- Sin `DreamcastRecomp ERROR`.
- Finalizó por timeout deliberado (`exit code 124`), no por crash.

Los logs y el `function_map` de esta validación están en `validation_lodoss_prelim/`.

## Cómo probar en Windows

```bat
run_lodoss_prelim_0.0.210.bat "C:\ruta\Record of Lodoss War.cdi"
```

También puede usarse `run_universal_0.0.210.bat` normalmente. El launcher preliminar activa automáticamente el burst START de prueba.

## Importante

El CDI del juego **no está incluido** en este paquete.
Este ZIP se guarda como checkpoint preliminar para retomar el trabajo desde aquí.
