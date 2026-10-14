# Organización de Dreamcast Recompiled v1.2

Se conservan los archivos críticos (`run_game.bat`, `CMakeLists.txt`, `runtime/`, `src/`, `include/`, `safe_guests/` y `portable/`) en sus ubicaciones originales. Los scripts históricos trasladados ajustan su directorio de trabajo y llamadas a otros BAT para que sigan apuntando a la raíz del proyecto.

## Scripts

- `scripts/stable/TEST_CPP38E_FAST.bat`: launcher rápido de la estable. El acceso recomendado es `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat` en raíz.
- `scripts/stable/TEST_CPP38E_HOTPC_DIAG.bat`: diagnóstico de hotspots (puede disminuir FPS durante medición).
- `scripts/stable/APPLY_CPP38E_HOT_BLOCK_RECOVERY.bat`: especialización puntual de guest al primer uso, invocado internamente por `run_game.bat`.
- `scripts/tests/`: pruebas históricas de versiones cpp34–cpp38, conservadas con sus opciones originales.
- `scripts/patches/`: parches antiguos; no ejecutes sobre una caché estable salvo que sepas lo que hacen.
- `scripts/maintenance/`: reparaciones y restauraciones de uso excepcional (algunas eliminan caché runtime, no el guest).
- `scripts/controller/`: prueba de mandos.
- `scripts/portable/`: inicio de portable ChuChu.

## Verificadores

Ejecutá `python tools/verification/legacy/VERIFY_CPP38E.py` desde cualquier directorio: verifica el código e invariantes de cpp38e. Usá `python tools/verification/check_layout.py` para verificar las rutas de los BAT de esta distribución.

## Compatibilidad

Ningún script histórico se ejecuta automáticamente desde el iniciador de la estable. No se cambian ubicaciones internas de `tools/`, los perfiles de juego, el guest ABI1, `runtime/`, `CMakeLists.txt` ni el directorio de caché del usuario.
