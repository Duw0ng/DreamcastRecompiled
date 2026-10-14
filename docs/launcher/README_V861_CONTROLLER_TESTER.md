# Dreamcast Recompiled v1.2 — Classic Aqua V8.6.1 Controller Tester

## Cambios de Mandos (sin alterar cpp38e ni V8.4 DXGI)

- Visualizador del mando Dreamcast basado en una ilustración frontal realista, sin depender de PIL/PyPI en Windows.
- Indicadores independientes de los gatillos **L y R** en los hombros y barras de presión (0–100%).
- Dos controles **XY completos** (stick izquierdo y stick derecho); eje Y positivo hacia arriba en la representación.
- Selector **P1–P4** persistente. El tester inspecciona solo el puerto escogido; no muestra por accidente otro mando conectado.
- Los resaltados de A/B/X/Y/Start, cruceta y L/R respetan las entradas de `profiles/controller_profile.ini`, incluidas asignaciones físicas `button:`, `pov:`, `axis:` y calibración `range:`.
- Resumen del perfil activo dentro del mismo panel, con nombres de asignaciones. El cambio de `controller_profile.ini` se detecta sin reiniciar el launcher.
- El editor del perfil y el Visual Tester externo original siguen disponibles. **P1–P4 selecciona el puerto que visualiza el tester; no modifica por sí solo el enrutamiento de los 4 puertos Maple dentro del juego.**
- Recursos y ventanas adaptados a los tamaños de launcher 800, 960 y 1152.

## Prueba

Extraer el ZIP en carpeta nueva, ejecutar `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`, abrir **Mandos**, seleccionar jugador y mover ambos sticks en círculo. Comprobar que los indicadores X/Y forman un recorrido circular y que Y se mueve verticalmente, no horizontalmente. Probar gatillos L/R y botones A/B/X/Y. El panel de perfil presenta las asignaciones realmente guardadas.

## Alcance y limitaciones

- En el modo `auto`, el tester consulta XInput del puerto elegido y, si está ausente, WinMM/DirectInput del mismo puerto. Se respeta `backend=xinput`, `backend=winmm` o `backend=ps4` al seleccionar explícitamente un backend.
- Los nombres físicos de botones WinMM dependen del controlador y del perfil aprendido en el Visual Tester. El mapeo básico se conserva para compatibilidad.
- La pantalla frontal de Dreamcast tiene **un stick físico**; el stick derecho del mando de PC se representa en el indicador XY adicional.
- La vibración del mando sigue dependiendo del runtime Maple existente. Las preferencias marcadas como pendientes en el launcher no activan funciones nuevas.
- No fue posible verificar con hardware PS4/PS5 real en Linux. Pruebas con datos sintéticos y GUI Tkinter bajo pantalla virtual.
- Ni los parámetros de video ni VSync fueron modificados.
