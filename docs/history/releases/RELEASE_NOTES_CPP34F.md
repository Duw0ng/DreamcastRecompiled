# DreamcastRecomp v1.2 cpp34f — ALIAS_SAFE_IPBIN_REENTRY_ABI1

## Corrección sobre cpp34e

cpp34e ya evitó la restauración prematura antes del primer frame, pero al aparecer
el primer flip podía armarse dentro del propio dispatch a `0x8C00FA00` y restaurar
IP.BIN sobre una relocación dinámica válida.

cpp34f cambia la política:

- El guard se arma **solo mientras se observa ejecución real en game RAM**
  (`0x8C010000+`) y ya existe al menos un page flip.
- Se elimina por completo el `just-in-time arming` desde un target IP.BIN.
- Si el target IP.BIN ya existe en `relocated_code_aliases`, la reentrada se
  **suprime** y el dispatcher normal conserva la relocación.
- El tick nunca restaura IP.BIN: solo observa/arma. La restauración queda
  restringida al dispatch de un target IP.BIN que no sea un alias vivo.
- Se mantiene el guard de targets SH-4 impares y el shadow de 32 KiB.
- Telemetría: `ipbin=valid/seen_game/armed/latched/restores/alias_suppressed`.

### Señal esperada en ChuChu Rocket

Después del primer flip, llamadas normales a 0x8C00FA00 deben producir:

`[DCR IPBIN] reentry suppressed | pc=0x8C00FA00 | live-alias=...`

y el juego debe continuar. Solo una entrada posterior a una dirección IP.BIN
**sin alias vivo** puede producir `[DCR IPBIN] confirmed reentry`.
