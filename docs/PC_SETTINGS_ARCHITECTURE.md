# PC Settings — arquitectura propuesta

El objetivo es exponer ajustes típicos de un port nativo sin acoplar el core de DreamcastRecomp al menú de un juego concreto.

## Capa host genérica

`DCRPCSettings` será propiedad del runtime y persistirá en un archivo de configuración junto al ejecutable. Campos iniciales propuestos:

- resolución de salida / tamaño de ventana
- fullscreen, borderless y windowed
- VSync / present interval
- escala interna de render (1x, 2x, 3x, 4x)
- aspect ratio (4:3 original, stretch, widescreen cuando el título/adaptador lo soporte)
- filtrado de textura (original/nearest, bilinear)
- volumen master, SFX y CDDA
- límite de FPS cuando sea apropiado

La capa host aplica cambios a PVR/D3D11/WinMM sin conocer direcciones guest ni estructuras de UI del juego.

## Adaptador de menú por título

Cada título puede registrar un pequeño `PCSettingsMenuAdapter` que inserte una entrada `PC SETTINGS` en su menú Options original y traduzca acciones guest a la API host. Esta parte sí es necesariamente específica de la UI del juego, pero no de la implementación de cada ajuste.

Para ChuChu Rocket! conviene primero localizar la tabla/estado del menú Options y su dispatcher de callbacks; después se añade una entrada adicional cuyo callback abre el submenú PC. El submenú puede comenzar como overlay host con estética similar al juego y, una vez estable, usar el renderer/textos del propio juego.

## Fallback universal

Todo runner puede ofrecer el mismo panel mediante una tecla/hotkey host aunque el juego todavía no tenga adaptador de menú. Esto permite desarrollar y validar resolución/VSync/render-scale una sola vez y luego integrar la entrada visual en cada título.

## Orden recomendado

1. cerrar CDDA actual; 2. retomar rendimiento; 3. crear `DCRPCSettings` + persistencia; 4. resolución/fullscreen/VSync; 5. overlay universal; 6. adaptador Options de ChuChu Rocket!; 7. render scale/aspect/filtering.
