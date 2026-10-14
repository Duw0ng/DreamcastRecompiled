# Dreamcast Recompiled v1.2 — Classic Original Launcher V5

**Interfaz funcional**, no un mockup ni una imagen utilizada como botón. Está implementada con Tkinter (incluido con Python estándar), con el arte separado en recursos PNG transparentes. Los botones, estados visuales, detección de clics, teclado, idioma, reloj y plegado del panel avanzado se dibujan y controlan en tiempo real.

## Corrección V5

El arte y los colores regresan al primer launcher. Ahora el mouse tiene un foco
exclusivo: el recorrido por espacios vacíos **no** reactiva la selección anterior.
Los botones tienen recursos «normal» e «iluminado» del mismo tamaño y se cambia
únicamente esa imagen, sin redibujar toda la ventana. También se eliminó la
superposición entre filas y los anillos avanzados se restablecen antes de cerrar
el panel. El reloj y todas las ventanas siguen siendo controles reales.

Ver `RELEASE_NOTES_CLASSIC_ORIGINAL_V5.md` para pruebas y límites.

## Pantalla principal

- Los **ocho botones grandes** funcionan: Jugar, Juegos, VMU/Archivos, Música, Ajustes, Mandos, Pantalla y Red.
- **Nueve botones avanzados**: Audio, Vídeo, Rendimiento, Guardados, BIOS/Sistema, Perfiles por juego, Diagnóstico, Idioma y Temas.
- **Brillo suave y centrado** cuando se pasa el mouse o selecciona un botón; ya no existe el rectángulo blanco descentrado.
- **Fecha/hora del PC**, actualizada cada segundo. El año 1998 no está incorporado en la pantalla actual.
- **Opciones avanzadas plegables**: se ocultan los nueve iconos y se reduce la altura física de la ventana. El estado se recuerda entre aperturas.
- Eliminada la barra inferior duplicada que tenía "Games" y "Play".
- Menús nativos en español e inglés y navegación con flechas, Tab, Enter o mouse.
- Tamaños pre-renderizados **1152, 960 y 800 px** según el escritorio, sin requerir Pillow en Windows.

## Funciones reales y límites

| Opción | Qué hace |
| --- | --- |
| Jugar | Inicia `scripts/stable/TEST_CPP38E_FAST.bat` reutilizando la caché del guest SH-4 |
| Juegos | Biblioteca CDI/ISO, agregar, elegir, quitar, jugar, perfiles y preparación SH-4 explícita |
| VMU / Guardados | Explora la carpeta persistente del sistema y hace copias de seguridad |
| Música | Reproduce WAV locales con WinMM en Windows; no altera audio CDDA de los juegos |
| Ajustes / Audio / Vídeo / Rendimiento | Cambia exclusivamente las opciones que el runner estable soporta |
| Mandos | Abre `Configurar_Mando.bat` de la versión estable y perfiles |
| Pantalla | Abre configuración de renderizador DX11/software sin inventar opciones de VSync/fullscreen |
| Red | Acceso a perfiles y registros; no promete conexión online funcional |
| Diagnóstico | Visor de logs y opciones de trazas |
| Perfiles por juego | Abre el `game.cfg` real en el editor del sistema |
| Idioma y temas | Persisten en la configuración del launcher |

La configuración se guarda en `%LOCALAPPDATA%\DreamcastRecompiled\launcher_v1_2\settings.json`. **No se modifica** el runtime cpp38e, el guest SH-4 ABI1, la caché ni los binarios.

## Abrir en Windows

Doble clic en `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat` (o `ABRIR_LAUNCHER.bat`). Requiere Python 3 con Tkinter. No se requieren paquetes PyPI. Si preferís consola, existe `INICIAR_DREAMCAST_RECOMPILED_V1_2_CLASICO.bat`.

El paquete incluye el proyecto original completo, no solo los archivos gráficos. Este front-end se verificó en Linux/Xvfb; falta la prueba de integración con juegos en el Windows del usuario.

## V7 — Ajustes extendidos

Abrí «Ajustes», «Pantalla», «Audio» o «Rendimiento» para ver los nuevos paneles nativos. Los cambios compatibles con cpp38e se aplican al lanzamiento siguiente; los ajustes futuros aparecen etiquetados «Pendiente de motor» y se guardan sin activar capacidades inexistentes. Las preferencias de **tamaño del launcher** son reales y necesitan reiniciar la interfaz para aplicarse. Encontrarás restauración del perfil estable y exportación/importación JSON en la pantalla de ajustes.

Documentación técnica: `TECHNICAL_VIDEO_INTEGRATION_V7.md`.

## Vídeo experimental V8 (sin VSync)

El panel `Configuración → Vídeo` permite seleccionar resolución de **salida**
(nativa, 960p, 1080p, 1440p, escritorio) y presentación 4:3 o 16:9.
El launcher transmite estas preferencias al runtime mediante variables
`DCR_VIDEO_RESOLUTION` y `DCR_VIDEO_ASPECT` **solo para el juego lanzado**.

No es resolución interna nueva del PVR ni widescreen real: 16:9 **estira**
la imagen Dreamcast 4:3. DX11 usa un pase de presentación GPU y software
utiliza StretchDIBits. En modo nativo 4:3 no activa el pase nuevo.
No se añadió VSync; `Present(0, 0)` permanece intacto.

El backend V8 aún requiere pruebas de compilación y gameplay real en Windows.
Si hay regresiones, restaurar `native / 4:3` sin modificar la caché SH-4.
Detalles: `RELEASE_NOTES_PRESENTATION_V8.md`.
