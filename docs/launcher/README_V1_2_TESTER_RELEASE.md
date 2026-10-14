# Dreamcast Recompiled v1.2 — Tester Release

Launcher Classic Aqua con biblioteca, vídeo DX11 1080p/16:9 y panel de mandos integrado.
La base de ejecución continúa en **cpp38e HOT_BLOCK_RECOVERY**. No se modificaron ni el
recompilador SH-4 ni la presentación DX11 que el usuario ya había comprobado.

## Mandos (sin abrir nuevas ventanas)

1. Abrí **Mandos** y elegí **Jugador P1–P4**.
2. En **Dispositivo**, elegí Auto, XInput 1–4 o DirectInput/WinMM 0–15. **Actualizar**
   vuelve a consultar qué dispositivos tienen señal. Se pueden escoger manualmente puertos
   sin señal, para diagnosticar controladores no identificados.
3. Pulsá **Iniciar** con los sticks centrados y los gatillos sueltos.
4. Seleccioná cada control, movelo hasta su extremo y pulsá **Capturar**. El proceso
   empieza con X/Y de los sticks y continúa con L y R; después podés aprender
   botones A/B/X/Y, START y cruceta desde el mismo selector. **Omitir** salta un paso.
5. Ajustá **Zona muerta** para corregir deriva. Pulsá **Guardar jugador** para guardar
   esa calibración en la configuración del launcher.
6. **Aplicar al juego** actualiza el archivo real
   `profiles/controller_profile.ini`, con copia de seguridad automática
   `profiles/controller_profile.ini.bak`.

Los dos indicadores de stick muestran X/Y en un mismo círculo, con ARRIBA como Y positivo
solo en la visualización. Las asignaciones se transforman a la convención original de Maple.
El tester usa el dispositivo seleccionado para ese jugador, sin sustituirlo de forma oculta.
Una calibración guardada para otro dispositivo no se reutiliza en el nuevo.

**Alcance real:** el runtime estable solo dispone de UN perfil activo de `controller_profile.ini`.
El launcher permite configurar y probar dispositivos independientes para P1–P4, pero
**Aplicar al juego** cambia el único perfil activo del motor; esta versión no habilita
cuatro asignaciones Maple simultáneas por arte de magia.

La calibración traduce movimientos a `axis:...`, botones a `button:...` o `pov:...`, y
presión de gatillos a `trigger:...` / `range:<eje>:<reposo>:<máximo>`, que ya comprende
el runtime de cpp38e. No existe soporte universal para cada variante de drivers DualSense;
cuando WinMM combine gatillos en un solo eje, calibrá L y R por separado.

## Para iniciar

`INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`

No es necesario recompilar el guest SH-4 por ajustar la interfaz o los mandos.
VSync no se modificó. El tester externo avanzado continúa disponible si lo necesitás.

## Comprobaciones

- 94 pruebas automatizadas del launcher, controles, perfiles y calibración en entorno virtual.
- Compilación de sintaxis Python correcta.
- Comprobación de igualdad byte por byte de todos los archivos externos a `launcher/`
  respecto al paquete V8.6.1, incluidos runtime, ejecutables y BAT.
- La detección física de mandos y el guardado con hardware real quedan pendientes de
  validación en Windows.
