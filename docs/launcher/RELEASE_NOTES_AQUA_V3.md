# Dreamcast Recompiled v1.2 — Aqua Launcher V3 (sin parpadeo)

## Motivo
El launcher V2 borraba todos los elementos del `Canvas` cada vez que el puntero cruzaba de botón (`motion() -> draw() -> canvas.delete('scene')`). Durante el redibujado llamaba a `root.update_idletasks()` después del borrado: en Windows podía aparecer el color azul del `Canvas` durante aproximadamente un segundo. Este comportamiento no estaba relacionado con el motor SH-4 ni con los FPS del juego.

## Correcciones
- Interfaz retenida: los iconos, botones, etiquetas y fondo se crean una sola vez. En hover, teclado, elección de juego y plegado del panel se cambia el estado de los elementos ya existentes con `canvas.itemconfigure()`, **sin borrar/reconstruir la escena**.
- La rutina de cambio de tamaño ya no fuerza `update_idletasks()` con la interfaz vacía.
- El foco ilumina el botón exacto; solo una selección está activa a la vez. Al ocultar las opciones avanzadas también se ocultan sus efectos de selección.
- El panel avanzado conserva su contenido en memoria al plegarse y despliega instantáneamente sus nueve controles.
- El reloj usa dos textos independientes para fecha y hora y la línea divisoria ya no invade los dígitos. La actualización es de 1 segundo, sin redibujar el dashboard.
- Retocado el alfa de los iconos originales para retirar restos del fondo azul: Jugar, VMU, Música, Ajustes, Mandos y Pantalla. En Temas se ha eliminado el fragmento de texto/botón que estaba adherido debajo del icono.
- Variantes de los PNG 1152/960/800 regeneradas desde los recursos corregidos; ninguna imagen nueva fue generada para este parche.
- La barra inferior duplicada continúa eliminada, y siguen activos Juegos, Ajustes, VMU, Audio, Mandos, Red, etc.

## Pruebas disponibles
- 20/20 pruebas de biblioteca y GUI en Tkinter/Xvfb (Linux).
- 1000 cambios consecutivos de hover: los IDs y el número de elementos de `Canvas` no cambian; no se destruye la pantalla.
- 800, 960 y 1152 píxeles: 17 centros de hit test, navegación, foco y apertura/cierre del panel.
- Comparación SHA-256 de todos los archivos fuera de `launcher/`: idénticos al paquete V2. **No se modifica cpp38e, runtime, SH-4, PVR, AICA, GD-ROM, BAT ni guest ABI1.**

## Windows
Extraer a una carpeta nueva y abrir `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`. No modificar manualmente la caché. Las pruebas de GUI se ejecutaron en Linux; confirmar personalmente la ausencia de parpadeo en Windows.
