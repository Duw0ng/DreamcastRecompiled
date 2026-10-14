# Dreamcast Recompiled v1.2 — Corrección biblioteca V8.1

## Error corregido

La biblioteca V8 creaba la tabla `ttk.Treeview` y el scroll como hijos de la ventana principal del diálogo y luego intentaba colocarlos en un `ttk.Frame` mediante `pack(in_=panel)`. Aunque la tabla tuviese filas, el panel opaco podía cubrirla; la ventana aparecía vacía.

Ahora el `Treeview` y su scroll se construyen directamente dentro de su marco contenedor (`ttk.Treeview(panel, ...)`, `ttk.Scrollbar(panel, ...)`) y se colocan usando `pack` estándar. La biblioteca lista tanto los juegos disponibles como las rutas de imágenes que ya no existen y proporciona un contador visible.

## Comportamiento

- **Juegos** muestra las entradas ya guardadas en `%LOCALAPPDATA%\\DreamcastRecompiled\\launcher_v1_2\\settings.json` sin importarlas otra vez.
- **Agregar CDI/ISO** incorpora el archivo seleccionado y lo muestra inmediatamente.
- **Añadir carpeta** incorpora archivos `.cdi` y `.iso` encontrados (máximo 500 por búsqueda).
- **Seleccionar** marca el juego para la siguiente ejecución; no recompila el SH-4.
- **Jugar** conserva la ruta de caché y los ajustes existentes. **Preparar SH-4** sigue siendo una acción explícita.
- **No encontrado** identifica imágenes movidas/eliminadas sin borrarlas silenciosamente.

## Verificaciones

- 52 pruebas automatizadas de launcher, incluidos cuatro tests nuevos de visibilidad y persistencia.
- Prueba gráfica en Xvfb: una entrada CDI aparece visible en el Treeview y en el contador.
- La V8 experimental de resolución/16:9 y el runtime cpp38e no se modifican.

## Nota

Las pruebas de interfaz se ejecutaron en Linux con Xvfb; hay que confirmar el resultado en Windows con un CDI real. No se copian ni recompilan los discos al agregarlos a la biblioteca.
