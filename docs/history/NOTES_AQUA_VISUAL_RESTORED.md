# Dreamcast Recompiled – Aqua visual restored

- `DreamcastRecompiled.exe` ahora es un lanzador gráfico Windows x64 con icono: abre la interfaz Aqua **original** `launcher/launch.py` (imágenes originales, iconos, navegación, mandos). Requiere Python 3 con tkinter como el flujo Python del proyecto. El binario nativo anterior se conserva como `DreamcastRecompiled_NativeAlt.exe` para casos sin Python.
- El launcher visual ahora utiliza `run_game.bat` en todos los modos, de forma que no omite caché incremental ABI1 ni recompila siempre desde 0.
- Red: automático, DCNet (puente módem/PPP existente) y off, vía `DCR_NETWORK_MODE`.
- Nuevo panel SH-4 y caché, presets `incremental`, `reuse`, `full` (este último es explícito), FPU, intérprete, seeds, patch DLL.
- Diagnóstico: consola externa y heartbeats desactivados por defecto; settings conservados.
- Imágenes e iconos originales de `launcher/assets` intactos. Los recursos `.png` no se embeben en el EXE; deben permanecer en el ZIP del proyecto.
- No se modifican componentes SH-4, runtime, plugin ni generador; solo `launcher/`, ejecutable principal y scripts de inicio.

**Limitación**: no es un empaquetado autónomo del intérprete Python. Windows necesita Python 3 con Tk para la GUI Aqua. No se ha ejecutado la GUI en Windows en este entorno.

También se incluye `BUILD_AQUA_STANDALONE_WINDOWS.bat` para generar en el PC del usuario el ejecutable autónomo con PyInstaller, conservando imágenes y Tk. El entorno Linux usado para generar este ZIP no permite compilar/verificar directamente esa variante Windows.
