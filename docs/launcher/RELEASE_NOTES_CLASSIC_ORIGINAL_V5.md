# Dreamcast Recompiled v1.2 — Classic Original Launcher V5

## Referencia artística

Regreso al primer diseño acuático de Dreamcast: encabezado gris clásico, fecha/hora reales, ilustraciones de mando, discos, VMU, nota musical, engranaje, monitor y red extraídas del arte original, y cápsulas de colores suaves. La interfaz **no** es una captura con zonas ocultas: conserva imágenes independientes, textos y acciones reales de Tkinter.

## Corrección del selector que mostraba el botón anterior

- La selección de mouse es **exclusiva**. Al salir del botón y pasar por un hueco queda sin iluminación; no se vuelve a encender el último foco de teclado.
- Las áreas de la fila superior y la inferior ya no se superponen.
- Cada cápsula tiene una única imagen Tk permanente con dos estados precargados y exactamente la misma geometría. El movimiento del mouse solo cambia la imagen del elemento existente: **no se reconstruye el Canvas**, no se limpian fondos ni se activan múltiples sprites transparentes.
- En el panel avanzado el anillo resaltado vuelve siempre al color normal antes de contraer el panel. Esto evita que sobreviva una selección al abrirlo nuevamente.
- Se abandonaron los sprites de halo antiguos, que podían generar una iluminación alrededor de iconos que no correspondía al botón activo.

## Conservación de la versión estable

El motor SH-4 ABI1, el PVR, audio, GD-ROM, portable y el runtime cpp38e permanecen byte por byte idénticos a la v1.2 Classic Aqua V4. La biblioteca y los ajustes de usuario se conservan en `%LOCALAPPDATA%`.

## Pruebas

28 pruebas de núcleo, GUI y recursos (Tkinter con Xvfb), tres tamaños (800, 960 y 1152), 2.000 transiciones alternadas en la interfaz y una comparación de capturas de pantalla en la que la región del botón «Jugar» regresa exactamente a los mismos píxeles después de recorrer las opciones. La integración visual definitiva en Windows requiere confirmación local.
