# Dreamcast Recompiled v1.2

**Edicion estable oficial — cpp38e HOT_BLOCK_RECOVERY (congelada el 7 de octubre de 2026).**

Esta entrega parte del archivo autentico `DreamcastRecomp_v1.2_STABLE_cpp38e_HOT_BLOCK_RECOVERY.zip`, verificado por SHA-256:
`1f3722dfa1f962c95b5d9594eb4f88dbb4a25860961869ef03a9bf1f5e4e7f06`.

## Para jugar desde Windows

Con PowerShell dentro de esta carpeta:

```powershell
.\INICIAR_DREAMCAST_RECOMPILED_V1_2.bat "C:\ruta\ccr.cdi"
```

Tambien se puede arrastrar un CDI sobre `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`.

La ejecucion utiliza el mismo `TEST_CPP38E_FAST.bat` de la version estable: `--reuse --spg-ff-on --hotpc-off --hot-recovery-on --no-portable-sync --no-pause`. Para cargar otro titulo por primera vez o generar el cache del guest, usa `run_game.bat "C:\ruta\juego.cdi"`, consultando antes `README.md`.

## Qué se mantiene intacto

- Codigo C++ de recompilador, SH-4, PVR heredado, AICA, GD-ROM, Maple y red original cpp38e.
- `run_game.bat`, `TEST_CPP38E_FAST.bat`, `APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat` y los demas BAT preexistentes.
- Guest ABI1 `safe_guests/CHUCHU_ROCKET/dcr_game_module.dll` y perfiles originales.
- Identificadores de recompilacion y cache (`DCR_RUNTIME_BUILD_ID`, `DCR_GUEST_IMPL_REV`): **no** se cambian por el nombre comercial.
- El modo base sigue con perfilador HOTPC desactivado por defecto.

## Cambios realizados para la presentacion

- Nombre visible de entrega, carpeta y lanzador: **Dreamcast Recompiled v1.2**.
- Textos visibles del lanzador portable `portable/DreamcastRecompiled.exe` recompilado desde Go y su fuente `portable/launcher/main_windows.go`. Solamente cambiaron cadenas de titulo/interfaz; se conserva la configuracion de carga interna de la version original.
- Este documento. Ningun cambio a las rutinas de juego, velocidad de reloj, renderizado o audio.

El ejecutable portable esta incluido, pero la prueba de rendimiento estable confirmada previamente corresponde al flujo `TEST_CPP38E_FAST.bat`. La recompilacion de la interfaz portable se valido cruzadamente para Windows x64, **no se ejecuto en Windows** aqui.

## Antes de reemplazar tu copia experimental

No mezcles archivos cpp38az/cpp38ba con esta carpeta. Extrae el ZIP en su propia carpeta. Los datos por juego se guardan en `%LOCALAPPDATA%\DreamcastRecompiled\games`; el codigo fuente cpp38e no borra esos datos por el simple hecho de extraer el ZIP. Si tienes una cache mezclada, respaldala antes de repararla, y evita `--clean` si deseas conservarla.

Historial tecnico del snapshot: `STABLE_BASE_CPP38E.md`, `CPP38E_HOT_BLOCK_RECOVERY.md`, `RELEASE_NOTES_CPP38E.md`.
