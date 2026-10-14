# Dreamcast Recompiled v1.2 — Classic Aqua Launcher V7

## Novedades

- Ventana avanzada con siete secciones: General, Vídeo, Audio, Rendimiento, Mandos, Sistema y Diagnóstico.
- Controles **reales** de cpp38e con etiquetas de compatibilidad: selector DX11/software/auto; audio on/off; HOT BLOCK RECOVERY; SPG Fast Forward; perfilador y trazas.
- Preferencias persistentes para el futuro backend de vídeo: resolución de salida, aspecto, filtro de escalado, fullscreen, VSync y overlay FPS. Se muestran **claramente como no funcionales en cpp38e**, no se añaden falsos flags de arranque.
- Preferencias de audio/mandos preparadas para APIs futuras, también claramente marcadas.
- Tamaño de ventana del launcher 800/960/1152/auto, aplicado al reiniciar y limitado por tamaño de monitor.
- Historial local de lanzamientos desde V7; nunca modifica la caché SH-4.
- Opciones individuales por juego para DX11/software, audio y preferencias futuras de vídeo.
- Restauración segura del perfil estable conservando juegos, VMU y caché SH-4.
- Exportación/importación del JSON de opciones del launcher, validado y filtrado.
- 41 pruebas automatizadas de core/GUI/resources incluyendo nuevos contratos de compatibilidad.

## Cómo abrir los ajustes

Ejecutar `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`; seleccionar Ajustes, Vídeo, Audio o Rendimiento desde la interfaz clásica.

## Compatibilidad

No hay modificaciones en `runtime/`, `src/`, el guest ABI1 ni los scripts estables. Los cambios se limitan al frontend `launcher/`, que mantiene los 17 iconos V6 y el fondo clásico.

Para una explicación de cómo implementar de verdad 1080p, VSync, 16:9 y fullscreen sin comprometer cpp38e, leer `launcher/TECHNICAL_VIDEO_INTEGRATION_V7.md`.
