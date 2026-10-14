# AQUA RC6: organización y host Python offline

La base técnica es RC5 (AQUA 3 THEMES DARKFIX). No cambia código SH-4, runtime, plugins, ABI, perfiles ni guardados. Se reorganizó documentación raíz y se retiró de arranque el descargador de Python.

Nuevo host: `launcher/offline_host.py` carga los archivos externos de `launcher/` en tiempo de ejecución. El workflow Windows construye `DreamcastRecompiled.exe` + `_internal/` con Python 3.12 + Tcl/Tk. En las siguientes versiones basta con conservar ese host y actualizar scripts y recursos; no hay reinstalación.

La compilación del host debe ejecutarse **una vez en Windows** (GitHub Actions o Python 3.12 previamente disponible). El ZIP de código fuente no incluye los binarios Windows; llamar 'offline listo' a ese ZIP sería incorrecto. El artefacto de Windows con `_internal/` es el release offline listo.
