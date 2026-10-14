# DreamcastRecomp v1.2 cpp34e — BOOTSAFE_IPBIN_REENTRY_ABI1

## Corrección de regresión cpp34d

cpp34d restauraba el shadow de IP.BIN demasiado pronto. Durante el boot normal de
ChuChu Rocket!, el bootstrap toca brevemente RAM >= 0x8C010000 y después ejecuta
helpers dinámicamente copiados en 0x8C00FA00 / 0x8C00F400. Ese tránsito no es una
reentrada de Homepage/DreamKey.

cpp34d interpretaba ese retorno como chain-load, restauraba los 32 KiB originales
sobre el helper recién copiado, limpiaba aliases y terminaba con 0x8C00FA00 en cero.

cpp34e conserva el shadow y el guard de destinos impares, pero añade un estado
BOOTSAFE:

- `seen_game`: se observó ejecución en RAM de juego.
- `armed`: solo pasa a 1 después de al menos un page flip real del PVR.
- Antes de `armed=1`, cualquier retorno a 0x8C008000-0x8C00FFFF forma parte del
  bootstrap inicial y NO restaura IP.BIN.
- Después de `armed=1`, una vuelta a IP.BIN sí se considera reentrada/chain-load.
- En la reentrada real se restauran los 32 KiB, dirty pages y aliases dinámicos.
- Se mantiene `[DCR SH4 GUARD]` para destinos impares como 0x8C00F9A3.

Heartbeat cpp34e:

`ipbin=valid/seen_game/armed/reentry_latched/restore_count`

Durante el boot esperado debe verse primero algo como:

`ipbin=1/1/0/0/0`

sin ninguna línea `reentry detected` al visitar 0x8C00FA00. Después del primer
frame aparecerá:

`[DCR IPBIN] reentry guard armed ...`

Y solo en una vuelta posterior a Homepage/DreamKey:

`[DCR IPBIN] reentry detected ... restored=32768`
