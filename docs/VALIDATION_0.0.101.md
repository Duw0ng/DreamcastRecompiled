# DreamcastRecomp 0.0.101 — validación

## Objetivo
Eliminar los impulsos aislados que permanecen en CDDA Yellow-Book sin alterar pistas RAW.

## Criterio de de-click
Solo en sectores `yb`, por canal, una muestra interior se interpola si su error frente al punto medio de sus vecinos supera 8192 y los vecinos difieren como máximo 2048.

## Telemetría
`cdda-src=.../xTRANS/rSYNC/dDECLICK/...`

## Ajustes PC
La arquitectura prevista separa un bloque host genérico persistente de un adaptador de UI por título, permitiendo insertar `PC SETTINGS` en Options sin contaminar el core con direcciones específicas del juego.

## Barrido del CDI de validación

En los 4294 sectores YB de T4, el criterio identifica 25397 muestras impulsivas (media ~5.9 por sector). Las diferencias consecutivas >8192 bajan de 58755 a 8317 en el barrido offline, ~86% menos. La mitad RAW de T4 y T5..T18 no pasan por el de-click.

## Build

- 47/47 CTest PASS.
- Un runner recién emitido fue configurado, compilado y enlazado por completo, incluyendo `dc_runtime.cpp` con `gd_cdda_repair_impulses()`.
