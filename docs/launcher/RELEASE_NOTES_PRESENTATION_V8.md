# Dreamcast Recompiled v1.2 — V8: salida 16:9 / 1080p (experimental)

Base: cpp38e HOT_BLOCK_RECOVERY. El guest ABI1, el módulo recompilado y la
emulación de dispositivos permanecen intactos. VSync sin modificaciones.

## Qué funciona en Windows

* `Resolución de salida`: nativa, 960p, 1080p, 1440p y escritorio.
* `Aspecto`: 4:3 conserva proporciones y agrega bandas; 16:9 usa una región de salida ancha
  estirando la imagen existente (con barras si la ventana tiene otro aspecto). **No es widescreen real ni aumenta el FOV**.
* DX11: pase final de triángulo completo con muestreo bilineal en la GPU;
  salida 640×480 intacta. En modo nativo sigue el `CopyResource` de cpp38e.
* Software/GDI: `StretchDIBits` hacia un rectángulo 4:3 o hacia la ventana entera.
* Por juego: los ajustes se heredan de la configuración global o del perfil
  del juego, sin alterar el archivo `dcr_game_module.dll` ni su caché.

## Qué NO incluye

* Renderizado PVR real a 1080p o nueva resolución de geometría/texturas.
* Cámara ampliada horizontalmente / parche widescreen por juego.
* VSync, fullscreen, cambiar el refresco o filtros alternativos.

La integración es optativa. La configuración por defecto no introduce nuevas
variables de entorno y conserva el trayecto original de cpp38e. Esta rama de
presentación necesita probarse en Windows y medir FPS/audio/cargas antes de
considerarse estable. No sustituye la v1.2 oficial congelada.

## Probar

Desde el launcher V8, seleccioná `Configuración → Vídeo → 1080p / 16:9`.
Probá primero `960p / 4:3`, luego `1080p / 4:3`, luego `1080p / 16:9`.
Si aparece un problema, volvé a `native / 4:3` y compartí el log.

Nota: Windows puede mostrar una ventana de 1080p demasiado grande si tu
monitor tiene una resolución inferior; la ventana se puede redimensionar.
