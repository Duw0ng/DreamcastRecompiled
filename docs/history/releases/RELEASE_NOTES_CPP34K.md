# DreamcastRecomp v1.2 cpp34k — STACKSAFE_IPBIN_REENTRY_ABI1

## Motivo

El log cpp34j confirmó que el boot estable volvió y que el FULLSTACK permanecía dormido, pero la reentrada de Homepage restauraba los 32 KiB completos de IP.BIN mientras el stack SH-4 vivo estaba dentro de `0x8C00Fxxx`. El crash final intentó ejecutar `0x41474553` (ASCII little-endian `SEGA`) con `SP=0x8C00F400`, señal de que la restauración estaba pisando estado vivo del chain-loader.

## Cambio

- Conserva páginas de 4 KiB dentro de IP.BIN que contengan el stack SH-4 vivo.
- Conserva también cualquier página que contenga aliases relocados (incluidos `0x8C00F400` y `0x8C00FA00`).
- Restaura el shadow inmutable de IP.BIN.
- Reinyecta las páginas dinámicas preservadas y mantiene su estado `literal_dirty`.
- Vacía solo el dispatch cache y conserva aliases de relocación.
- Telemetría nueva: `sp`, `pr`, `live-page-mask`, `restored-static-pages`, `live-pages-preserved`.
- No cambia `DCRuntime` ni `DCR_GUEST_ABI=1`.
- El FULLSTACK continúa lazy y no participa del boot hasta el primer MMIO real del módem.

## Prueba

Usar `TEST_CPP34K_HOMEPAGE_STACKSAFE.bat`. Ese BAT pasa `--reuse`; si no existe el guest ABI1 ya compilado, aborta en lugar de recompilar ChuChu.
