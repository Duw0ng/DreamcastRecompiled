# Dreamcast Recompiled v1.2 — Win32 Native Aqua (primera integración)

## Qué se entrega

* `DreamcastRecompiled.exe`: **PE32+ GUI x64**, con icono Dreamcast en recursos. Interfaz Win32/GDI+ compilada en Go (código máquina Windows), **sin Python, Tkinter, PyInstaller, WebView ni consola** para abrirse.
* Menú Aqua con arte original `launcher/assets/dreamcast_launcher_1152.png` (redimensionado en pantallas menores), botones interactivos, fecha/hora actual, biblioteca nativa CDI/GDI/ISO, selección de juego, mandos y rumble, ajustes de DCNet/PPP, DX11/software, audio, FPU, heartbeats, trazas y recompilación SH-4 incremental.
* Conversión de la configuración básica desde `%LOCALAPPDATA%\DreamcastRecompiled\launcher_v1_2\settings.json` hacia `%LOCALAPPDATA%\DreamcastRecompiled\launcher.json` **solo al primer arranque** si no existe configuración nativa.
* `tools/native/dcr_native_tools.exe`: utilidad Windows x64 nativa para `abi`, `sha256`, `cmake-home`, `runtime-verify`, `native-seeds`, `modularize`, `metricfree`, `hot-recovery`, `portable-sync` y `adopt-safe`. `run_game.bat` utiliza estas herramientas cuando el EXE está presente. Python queda como respaldo únicamente si falta el EXE.
* Las reglas de *native incremental seed DLL*, sus hashes de imagen y su cuarentena de seeds permanecen; la DLL SH-4 base no se sobrescribe por descubrir una seed.

## Advertencias y pruebas pendientes

1. El launcher nativo es una **nueva implementación** de la interfaz, no un empaquetado de Python. El menú conserva el arte, pero algunas pantallas secundarias aún no son equivalentes a las páginas detalladas de Tkinter. Música, idioma y temas conservan accesos señalizados como pendientes, y la versión legacy sigue disponible en `launcher/launch.py`.
2. **Se requiere Windows 10/11 x64**. Las bibliotecas Win32 y GDI+ vienen con Windows; no hay que instalar Python para abrir ni para el flujo normal de preparación con los ejecutables incluidos. Para recompilar nuevo código de juego siguen siendo necesarios **MSVC + CMake**, como antes.
3. La prueba del módulo Go se realizó en Linux y se compiló Windows x64 por cross-compilation. Las transformaciones C++ generadas por `modularize`, `metricfree` y `hot-recovery` se **compararon byte por byte** con las salidas de Python en una carpeta de prueba sintética: coincidieron. **No se ha ejecutado ni probado en Windows real** ni se han validado juegos comerciales en esta revisión.
4. Si surge un error en el flujo nativo de preparación, ejecutá temporalmente el runner anterior y adjuntá log. Los scripts Python originales quedan intactos para comparación; no afectan al arranque del nuevo launcher.
5. El `.exe` principal necesita la carpeta `launcher/assets`; mantené la estructura completa del ZIP. El icono **sí está incrustado** en el ejecutable.

## Uso

1. Extraé el ZIP en una **carpeta nueva**; no sobrescribas tu instalación estable.
2. Abrí `DreamcastRecompiled.exe` directamente. También funcionan `ABRIR_LAUNCHER.bat` y `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`, ahora apuntando al EXE.
3. **Juegos** → agregá tu CDI/GDI/ISO → Seleccionar. Luego **Jugar**.
4. **Red** → DCNet/Flycast compatible; **Compilación SH-4** → Incremental automática.
5. En **Opciones** podés cambiar vídeo/audio/FPU, heartbeats, consola, control y logs. El archivo `launcher.json` guarda la selección, no el código SH-4.
6. Para abrir el tester de controles directamente: `Configurar_Mando.bat`.

## Compilar nuevamente (solo desarrolladores)

`BUILD_WIN32_NATIVE_AQUA_WINDOWS.bat` compila con Go 1.23+ los dos ejecutables; no requiere instalar Python. El runtime Dreamcast sigue siendo C++20.

## Tests efectuados

- `go test ./...` para launcher y herramientas.
- Cross-build Windows PE32+ x64 del launcher GUI y de `dcr_native_tools.exe`.
- Confirmación de recurso `.rsrc` (icono integrado).
- Comparación de ABI fingerprint con el Python original.
- Prueba de equivalencia del guest modularizado, parche metric-free y especialización cpp38e frente a Python (archivos iguales en fixture).
- Manifest de runtime: escritura, reutilización válida y rechazo cuando cambia código fuente.
- Seeds SH-4: filtrado de offsets inválidos, dinámicos comentados y duplicados.

No se afirma validación MSVC/Windows ni conexión DCNet nueva todavía.
