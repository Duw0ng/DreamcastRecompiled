# DreamcastRecomp v1.2 cpp34d — IPBIN_REENTRY_SHADOW_GUARD_ABI1

## Objetivo

Esta build parte directamente de cpp34c `FLASH_BASE0_FIX_ABI1` y conserva ABI guest v1.
No vuelve a recompilar la closure SH-4 si el cache ABI v1 ya existe.

## Cambios

- Captura una copia inmutable completa de IP.BIN (`0x8C008000-0x8C00FFFF`, 32 KiB)
  durante `dc_setup_commercial_boot`.
- Detecta retorno desde código de juego (`>=0x8C010000`) hacia IP.BIN tanto desde
  el tick completo como desde dispatch dinámico.
- Restaura IP.BIN antes de ejecutar la reentrada.
- Limpia las páginas de literales dirty correspondientes.
- Vacía el dispatch cache y elimina aliases dinámicos cuyo destino solicitado está
  dentro de IP.BIN, evitando reusar relocaciones descubiertas cuando low RAM estaba sobrescrita.
- Rechaza destinos SH-4 impares con `[DCR SH4 GUARD]` en vez de tratarlos como un
  missing target recompilable.
- Añade fingerprint FNV-1a del shadow para comprobar en el log que la restauración
  coincide con la imagen capturada.
- Heartbeat inequívoco:
  `build=v1.2-cpp34d-ipbin-reentry-shadow-guard`
- Telemetría:
  `ipbin=valid/seen_game/reentry_latched/restore_count`
- Plugin de red se identifica como `cpp34d-network-abi1`; ABI y comportamiento de
  red no cambian respecto de cpp34c.

## Test recomendado

Arrastrar `ccr.cdi` sobre `TEST_CPP34D_IPBIN.bat`.

El BAT fuerza `--rebuild-runtime`, recompila `DreamcastRuntime`, `dcr_network` y
el pequeño runner, pero conserva el módulo SH-4 guest ABI v1.

En el nuevo log deben aparecer:

- `[DCR BUILD] runtime=v1.2-cpp34d-ipbin-reentry-shadow-guard`
- `[DCR IPBIN] immutable shadow captured`
- `build=v1.2-cpp34d-ipbin-reentry-shadow-guard`
- al volver a Homepage/browser: `[DCR IPBIN] reentry detected ... restored=32768`

Si vuelve a aparecer `0x8C00F9A3`, el guard lo reportará explícitamente como
`target-align=odd-invalid-sh4`, lo que permitirá saber si la corrupción ocurre
antes o después de la restauración.
