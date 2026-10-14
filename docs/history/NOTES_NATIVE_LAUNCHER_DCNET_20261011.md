# Dreamcast Recompiled v1.2 — Native Windows Launcher (DCNet)

## Entregable

`DreamcastRecompiled.exe` en la **raíz**, un ejecutable GUI nativo Win64 (Go/Win32), con recurso de icono ICO embebido: no es un acceso directo ni un script. `ABRIR_LAUNCHER.bat` e `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat` lo prefieren. No requiere Python para abrir la ventana, pero el motor/runner sigue necesitando sus herramientas de build según corresponda.

## Cambios funcionales

- Abrir CDI / GDI / ISO, ajustes persistentes en `%LOCALAPPDATA%\DreamcastRecompiled\launcher.json`.
- Red: `Auto (PPP)`, `DCNet / Flycast compatible`, `Sin conexión`. Con `DCNet`, el runtime usa el puente de módem V.42/LAPM/PPP existente en `dcr_network.dll`. No añade un servidor ni certifica conexión pública; corresponde probarlo con un servidor real. Al desactivar, `DCR_NETWORK_MODE=off` pone `network_enabled=false` sin alterar layout ABI1.
- Compilación SH-4: `Incremental automática`, `Reutilizar caché`, `Recompilación completa`. La última se resetea después de lanzar, para evitar recomputar la caché en ejecuciones siguientes. Todos los lanzamientos desde la raíz pasan por `run_game.bat`, preservando el sistema de seeds y DLL ABI1.
- Mando: se conserva el editor por puertos A-D y el tester visual con rumble del launcher Go original.
- Opciones: renderer dx11/software, audio completo/básico/desactivado, reloj host/guest; perfil de rendimiento, debug, PVR, Maple, GD-ROM; SPG, HOTPC, Hot Recovery, FPU, interpr. fallback, trazas, heartbeats, consola, sincronización portable y rebuild modular de plugins.
- Vídeo: resolución nativa, 960p, 1080p, 1440p, pantalla completa de tamaño escritorio; proporción 4:3 o 16:9 estirada, salida DXGI o GDI compatibilidad. *16:9 no amplía FOV ni modifica el render del guest*.
- Rebuilds de plugins y recompilación completa son de un solo uso. NativeSeeds/interpreter permanecen habilitados por defecto. Opciones persistentes.
- La consola externa está desactivada por defecto y las salidas se registran en `%LOCALAPPDATA%\DreamcastRecompiled\logs`. Cuando se pide consola visible se genera un wrapper local `.cmd` y se abre una consola de Windows con salida de texto visible.

## Alcance y límites

- No se ha ejecutado todavía el `.exe` ni la sesión DCNet en Windows real en este entorno. Se comprobó la **compilación cruzada Go Win64**, la presencia del icono en `.rsrc`, `go test`, `go vet`, compilación del plugin red y pruebas Linux del runtime.
- El código exacto de `cpp38h2 DUAL_DIRECT_DCNET` (anterior sesión exitosa de 2 instancias) no se importó: el proyecto actual contiene el plugin V.42/LAPM/PPP `cpp34s`. DCNet en el launcher es el perfil explícito para dicho puente existente, que podría requerir correcciones si hay diferencias respecto a cpp38h2. No se reemplazó el plugin por uno no probado.
- `portable/DreamcastRecompiled.exe` sigue siendo el ejecutable **legado** para testers portables antiguos; el EXE **nuevo** es `./DreamcastRecompiled.exe` en la raíz, porque necesita `run_game.bat` para construir los parches incrementales.
- No se anuncian Vulkan, OpenGL, DX12 o VSync como funciones implementadas.

## Prueba sugerida (Windows)

1. Descomprimir la carpeta completa y ejecutar `DreamcastRecompiled.exe` en su raíz.
2. Seleccionar un CDI ya validado (ChuChu Rocket!), compilación `Incremental automática`, Red `DCNet / Flycast compatible`.
3. Iniciar y comprobar `dcr_network.dll` en los logs; con una seed nueva en `interp_seeds.txt`, confirmar `[NATIVE SEEDS]` en el siguiente arranque. No debería recompilar el guest nativo anterior.
4. Repetir con `Sin conexión` y comprobar `[NET] network disabled by launcher` y ausencia de conexión.
5. Probar `Opciones avanzadas` para activar/desactivar la consola y verificar que el ajuste persiste en `launcher.json`.
