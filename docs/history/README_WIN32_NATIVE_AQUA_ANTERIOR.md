# Dreamcast Recompiled v1.2 — Aqua nativo Win32 (experimental)

**Novedad:** `DreamcastRecompiled.exe` es un ejecutable Windows x64 **nativo Win32/GDI+ (Go compilado)** con el arte Aqua original e icono incrustado. Abrir el launcher **ya no requiere Python**; el runner usa `tools/native/dcr_native_tools.exe` para las tareas del compilador que antes necesitaban scripts Python, incluso al preparar un guest nuevo. Python solo permanece como respaldo legacy si falta la herramienta nativa. MSVC/CMake siguen siendo necesarios para compilar juegos.

**IMPORTANTE:** las pantallas secundarias Música/Idioma/Temas no tienen todavía la misma funcionalidad que el launcher Tkinter; esta build no está validada en Windows. Ver [NOTES_WIN32_NATIVE_AQUA_20261011.md](NOTES_WIN32_NATIVE_AQUA_20261011.md) antes de utilizar.

---

La pantalla principal es una aplicación Tkinter interactiva: botones iluminados, hora real, menú avanzado plegable, navegación con teclado, biblioteca CDI/ISO y todas las opciones respaldadas por el runtime estable. **No es una ilustración estática**. Ver [launcher/README.md](launcher/README.md).

Doble clic: `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`.

---

# Dreamcast Recompiled v1.2

**Versión oficial estable** — base **cpp38e HOT_BLOCK_RECOVERY**.

Esta distribución reorganiza los scripts y documentos sin introducir cambios de la rama experimental cpp38ay–cpp38ba. Conserva el runtime, recompilador, guest ABI1 y caché compatible con cpp38e.

## Inicio rápido (Windows)

**Nuevo launcher Aqua**: doble clic en **`INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`** (o `ABRIR_LAUNCHER.bat`). Se abrirá la interfaz retro-acuática con biblioteca y ajustes. El lanzador usa Python 3 con Tkinter, sin paquetes externos.

Desde la interfaz, **Games** permite agregar imágenes `.cdi` / `.iso` y **Play** inicia el juego. Podés arrastrar un disco sobre el BAT para dejarlo seleccionado.

**Importante**: Play llama al BAT estable cpp38e con `--reuse`, sin recompilar SH-4. Si es la primera vez que abrís ese juego y aún no existe un guest compatible, usá **Games → Preparar SH-4** (con confirmación explícita).

Para utilizar el inicio clásico por consola sin GUI, ejecutá:

```powershell
.\INICIAR_DREAMCAST_RECOMPILED_V1_2_CLASICO.bat "C:\ruta\ccr.cdi"
```

Para ejecución avanzada: `run_game.bat "C:\ruta\juego.cdi" --reuse`. [Documentación completa del launcher](launcher/README.md).

## Organización

| Carpeta / archivo | Contenido |
| --- | --- |
| `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat` | Nuevo launcher visual Aqua |
| `ABRIR_LAUNCHER.bat` | Acceso directo alternativo a la GUI |
| `INICIAR_DREAMCAST_RECOMPILED_V1_2_CLASICO.bat` | Lanzador clásico cpp38e, sin GUI |
| `launcher/` | GUI Tkinter, imágenes, pruebas y gestor de biblioteca |
| `run_game.bat` | Runner universal; permanece en raíz por compatibilidad |
| `build_windows.bat` | Compilación del proyecto en Windows |
| `Configurar_Mando.bat` | Configuración del mando |
| `run_tests.bat` | Pruebas CMake/CTest |
| `scripts/stable/` | Lanzadores cpp38e y especialización HOT BLOCK RECOVERY |
| `scripts/tests/` | Pruebas históricas (diagnóstico / desarrollo) |
| `scripts/patches/` | Parches históricos; **no ejecutar en la estable sin motivo** |
| `scripts/maintenance/` | Diagnóstico, restauración y mantenimiento de cachés |
| `scripts/controller/` | Pruebas de mandos |
| `scripts/portable/` | Lanzadores portable |
| `tools/verification/legacy/` | Verificadores Python de versiones anteriores |
| `docs/stable/` | Documentación de la base oficial |
| `docs/history/` | Notas, validaciones y changelogs históricos |
| `docs/reference/` | README técnico detallado y material de referencia |
| `runtime/`, `src/`, `include/` | Código C++ **sin modificar** |
| `safe_guests/`, `portable/` | Binarios preexistentes **sin modificar** |

## Consideraciones

- No borres `%LOCALAPPDATA%\DreamcastRecompiled\games`: ahí se guardan las cachés y perfiles de los juegos.
- La primera ejecución puede compilar componentes que falten; la caché ABI1 existente se conserva siempre que sea compatible.
- Los scripts de `scripts/tests` y `scripts/patches` reproducen versiones históricas: son herramientas para desarrolladores, no parte del inicio normal.
- No uses los scripts experimentales cpp38ay–cpp38ba en esta versión estable.

### Intérprete SH-4 de respaldo

Si el juego salta a una dirección SH-4 que el recompilador no compiló, el runtime la **interpreta** en lugar de cerrar el juego, y la guarda en `interp_seeds.txt` para compilarla en la próxima "Preparar SH-4". Está activo por defecto; `--no-interp` restaura el comportamiento anterior. Ver [docs/SH4_INTERPRETER_FALLBACK_v1.2.md](docs/SH4_INTERPRETER_FALLBACK_v1.2.md).

Guías adicionales: [LEEME PRIMERO](docs/stable/LEEME_PRIMERO_V1.2.md) · [Descripción original](docs/reference/README_TECNICO_ORIGINAL.md) · [Cambios históricos](docs/history/CHANGELOG_COMPLETO.md) · [Estructura detallada](docs/stable/ESTRUCTURA_DEL_PROYECTO.md).

### Rama opcional: salida 1080p / 16:9 — V8

El launcher ofrece escalado de presentación 960p/1080p/1440p y un modo
16:9 estirado. El PVR continúa renderizando internamente a 640×480; no se
amplía el FOV del juego. VSync no se implementó. Ver instrucciones en
`launcher/RELEASE_NOTES_PRESENTATION_V8.md`. La configuración nativa 4:3
reproduce el camino de presentación de cpp38e.
