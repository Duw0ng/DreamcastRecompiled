# Dreamcast Recompiled v1.2 AQUA — candidato RC1 (13/10/2026)

## Cambios visibles de esta edición
- Se elimina el nombre interno histórico de la identidad pública: `v1.2 AQUA`.
- Menú Aqua con **tema nocturno**. El menú Tkinter cambia fondo, cabecera y páginas;
  el launcher Win32 incluye arte nocturno y guarda su preferencia.
- **Administrador de VMU**: inspector de imagen real de 128 KiB con uso libre,
  entradas de directorio, cadenas FAT y estado de integridad. El Aqua Python
  ofrece exploración, refresco y respaldo; el nativo `VMU_Manager.exe` permite
  ver el diagnóstico y arrastrar un .bin para leerlo.
- `DreamcastRecompiled.exe` es ahora el launcher **nativo de Windows ya
  compilado**, en lugar del preparador obligatorio de Python.
- El preparador previo sigue disponible, opcional, como
  `DreamcastRecompiled_Aqua_Setup.exe`. El Aqua Python no contiene un runtime
  Windows embebido en este ZIP y todavía requiere una preparación con Internet.

## Regresiones que se deben volver a probar
1. Sega Tetris: boot, partida, audio, menús y salida.
2. ChuChu Rocket!: juego, estabilidad y controles.
3. Mr. Driller: gameplay y comparación de fallas gráficas ya conocidas.
4. SGGG: acceso a menús y fallas de audio conocidas.
5. VMU: abrir/respaldar `dreamcast_vmu_a1.bin`, cerrar el juego y volver a abrirlo.
6. Dos aperturas del launcher: tema persistente y selección de CDI.

## En revisión (no bloquear el lanzamiento por una etiqueta engañosa)
- No se garantiza que Toy Commander abra: el parser CDI de esta estable sigue
  presentando errores con su variante de DiscJuggler.
- Mars Matrix crashea durante inicio según las pruebas previas.
- Resident Evil Code: Veronica y Sonic Adventure pueden consumir mucha RAM al
  generar/compilar el código C++. Los experimentos STREAM RAM y BYTE SHARDS
  quedan excluidos de esta rama estable a propósito.
- LCD de VMU 48×32 en tiempo real: **no implementado**. El dibujo de pantalla
  en el menú Aqua indica estado, no contenido del juego.
- La GUI nativa Win32 ofrece menos detalle que el Aqua Python en algunos menús.
- Cross-build Windows x64: ejecutables compilados aquí, no ejecutados en una
  sesión gráfica de Windows con juegos comerciales.

## Compatibilidad conocida (reportes de pruebas, no certificado general)
- ChuChu Rocket!: funcional.
- Sega Tetris: funcional; gráficos y sonido correctos, un crash previo aislado.
- Mr. Driller: jugable, fallas gráficas importantes en menús.
- SGGG: arranque y funcionamiento parcial con anomalías de audio.
- Toy Commander: parser CDI falla.
- Mars Matrix: crash de inicio.
- Power Stone: aún no testeado.

## Integridad y automatización
- Motor nativo, runtime, plugins, recompiler y BAT de inicio conservados frente
  al ZIP estable, excepto textos informativos (nombre de versión, título de ventana y mensajes).
- Nuevos archivos: `launcher/vmu_inspector.py`, `launcher/vmu_ui.py`,
  `tools/vmu_manager`, imágenes nocturnas y binarios GUI Win32.
- Tres pruebas nuevas del parser VMU pasan. Tests GUI de arranque nocturno y
  navegación VMU pasan en Xvfb. Suite heredada: 125/130, con cinco fallas de
  pruebas antiguas ya existentes en la rama estable.
- No se incluye juego, BIOS ni imagen VMU del usuario.

## Próximas tareas sugeridas antes de GitHub Release estable
- En Windows real: comprobar MSVC/CMake, guardar logs y adjuntar diagnóstico.
- Comprobar teclas/foco/controles desde ventanas activas e inactivas.
- Probar persistencia VMU con dos sesiones de juego; implementar pantalla LCD
  real mediante intercambio IPC con runtime si se confirma necesidad.
- Empaquetar Aqua Python + Tcl/Tk offline desde un build Windows confiable.
- Añadir reporte de compatibilidad con versión, renderer, FPS y reproducción.

## Historia técnica
Se conservan logs y anotaciones históricos de las iteraciones `cpp38` y mejoras
(entre otras: `HOT_BLOCK_RECOVERY`, SPG y HOTPC) en `CHANGELOG.md` y `docs/`.
Los identificadores ABI1 y sellos internos no se renombran para proteger cachés.
