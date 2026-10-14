# Dreamcast Recompiled v1.2 — Selector sencillo de dispositivos

El apartado **Mandos** conserva el mismo fondo Aqua, el mando Dreamcast en vivo y P1–P4.

En **Dispositivo** aparecen juntos:

- **Automático** (mapea el mando disponible en el puerto correspondiente).
- **Desconectado** (puerto Dreamcast sin mando).
- **Teclado + Mouse** (un solo control combinado; no necesita calibración).
- **Mandos XInput conectados** (por puerto físico).
- **Mandos DirectInput/WinMM conectados**, indicando el nombre que reporte Windows cuando esté disponible.

**Buscar mandos** actualiza la lista de dispositivos detectados. Si desconectás el que habías elegido, su selección sigue visible como «Mando no conectado» hasta que vuelvas a buscar o elijas otro. Un mismo pad expuesto por Windows simultáneamente a XInput y WinMM podría aparecer dos veces; no se pueden considerar dispositivos distintos sin un identificador PnP confiable.

La elección de P1–P4 se guarda por jugador. **Al iniciar un juego desde el launcher**, se envían las variables `DCR_MAPLE_PORT_A_DEVICE` hasta `DCR_MAPLE_PORT_D_DEVICE`, admitidas por el plugin Maple cpp36b que todavía forma parte de v1.2. Los valores son `auto`, `none`, `keyboard_mouse`, `xinput:N` o `winmm:N`. El valor predeterminado sigue siendo A=auto y B/C/D=none. Los cambios se aplican al próximo inicio de un juego, no durante su ejecución.

El tester visual de **Teclado + Mouse** enseña las mismas teclas de compatibilidad que el runtime: WASD (stick izquierdo), flechas (cruceta), J/K/U/I (botones), Q/E (gatillos L/R), Enter (Start), y clics del mouse para botones. No hace falta asignarle un backend ni calibrarlo. El calibrador avanzado de ejes, gatillos y botones permanece disponible para mandos físicos y guarda sus perfiles sin alterar la caché SH-4.

**Importante:** Maple admite la selección de dispositivos por puerto, pero las calibraciones INI personalizadas siguen compartiendo un único perfil activo; no se crearon falsos perfiles de calibración independientes en el runtime. El selector combinado y los cuatro puertos son efectivos cuando lanzás el juego mediante la interfaz nueva. Si usás los BAT clásicos directamente, éstos siguen sus opciones originales.

No se alteraron runtime, guest ABI1, DX11 1080p, VSync ni scripts BAT existentes.
