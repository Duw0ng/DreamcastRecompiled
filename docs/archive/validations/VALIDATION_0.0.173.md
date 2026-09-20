# DreamcastRecomp 0.0.173 — Crazy Taxi 2 title/menu closure

## Objetivo

Convertir el avance visual hasta el menu real de Crazy Taxi 2 en una build testeable manualmente, sin los probes usados para acelerar o automatizar la investigacion.

## Compatibilidad incorporada

- Conserva el cierre post-VMU de 0.0.172 (`0x8C0372A0`, `0x8C037790` y dependencias limpias).
- Registra el target post-title `0x8C07D9AA`.
- Registra `0x8C03DC5C` y sus bloques internos.
- Registra los helpers dinamicos que desbloquearon el menu: `0x8C048E6E`, `0x8C033E38`, `0x8C03457C`, `0x8C034928`.
- Mantiene los fixes anteriores de BRAF, ARM7 PC+12, boot comercial, modem/G2 y literales PC-relativos mutables.

## Limpieza para la build del usuario

- Sin fast-forward de rasterizado.
- Sin START automatico en el title screen.
- Sin A/YES automaticos en las pantallas de VMU/save.
- Sin watchpoints CT2 de input/VMU usados durante la auditoria.
- Input manual mediante teclado/XInput por Maple host input.

## Milestone observado durante desarrollo

Con la misma cobertura SH-4, usando probes solo para atravesar rapidamente las pantallas durante la investigacion, Crazy Taxi 2 alcanzo visualmente:

`NOW LOADING -> VMU/save -> Presented by SEGA -> PRESS START BUTTON -> MODE SELECTION`

La captura de `MODE SELECTION` se obtuvo tras ~320k paquetes PVR, sin missing-target antes del menu.

## Limitacion conocida

La reconstruccion de estado de `syMallocInit` continua siendo provisional en esta salida experimental. Los allocators posteriores son las rutinas SH-4 guest recompiladas del juego.

## Validacion ejecutada antes del ZIP

- Build Release/Ninja del proyecto principal: **PASS**.
- CTest del proyecto principal: **50/50 PASS**.
- Build completa de `experimental_ct2_0.0.173` con Clang: **PASS**.
- `generated_compile_test`: **RC=0**.
- Smoke comercial sin auto-input: llego a **1000 paquetes PVR**, 50 solicitudes GD-ROM / 3519 sectores / 7,206,912 bytes y termino con **RC=2** exclusivamente por `--pvr-stop-after-packets 1000`.
- El heartbeat del smoke reporto `build=0.0.173`.
- Escaneo final: sin `CT2-UI-SYNC-A`, `CT2-CONFIRM-YES-A`, `CT2-YES-CONFIRM`, `CT2-VMU-PATH`, fast-forward de menu ni START automatico.
