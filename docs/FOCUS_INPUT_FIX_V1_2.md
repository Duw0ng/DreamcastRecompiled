# Dreamcast Recompiled v1.2 — Input Focus Fix (Tester Release)

## Qué se corrigió

- El plugin Maple ABI2/V3 ya no procesa teclado, mouse, XInput ni WinMM cuando la ventana Dreamcast no es la ventana activa de Windows.
- La misma protección existe en la ruta de compatibilidad del runtime.
- El bloqueo funciona para los puertos Dreamcast A–D, con selector de P1–P4.
- Los controladores permanecen conectados, pero sus botones, sticks y gatillos devuelven cero al juego mientras éste está en segundo plano.
- Se descarta la posición anterior del mouse al perder el foco para evitar saltos al volver al juego.
- La protección comprueba **el PID y la clase real** de la ventana `DreamcastRecompPVRWindow`, en lugar de confiar en un HWND no inicializado dentro del plugin.

## Cómo probar

1. Extraer esta distribución en una carpeta nueva.
2. Abrir el launcher, asignar el dispositivo que se desee en **Mandos** y ejecutar ChuChu Rocket!.
3. Con la ventana del juego activa, probar teclado, mouse o mando: los botones deben funcionar.
4. Cambiar a otra aplicación con Alt+Tab o hacer clic en el escritorio. Mientras el juego esté inactivo, las teclas y botones de mando no deben afectar al juego.
5. Volver a la ventana del juego: debe aceptar entradas de nuevo y el mouse no debe generar un salto acumulado.
6. Repetir para los puertos B–D si se usan varios dispositivos.

Los logs de la ejecución podrán incluir estas transiciones:

    [MAPLE FOCUS] inactive | input=neutral A-D
    [MAPLE FOCUS] active | input=enabled

**No se ha modificado el guest SH-4, su caché ABI1, la ruta gráfica DX11, el audio ni VSync.** El primer arranque actualizará las DLL de runtime/Maple, sin generar nuevamente el módulo SH-4 del juego.

## Verificaciones de esta entrega

- Compilación Linux Release de DreamcastRuntime y de los plugins actualizados.
- 53 pruebas CTest aprobadas (se excluyeron dos pruebas largas de generación completa).
- Test de regresión `maple_input_focus_tests`: foco propio, fondo, ventana ajena y minimizado.
- Integridad del ZIP. La comprobación visual y de entrada física en Windows sigue pendiente.
