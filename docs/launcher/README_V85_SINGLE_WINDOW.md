# Dreamcast Recompiled v1.2 — Aqua Launcher V8.5

La pantalla principal y sus **17 iconos** originales permanecen igual.

## Navegación

- Seleccioná Juegos, Vídeo, Audio, Mandos, VMU, etc. y el contenido se abre
  **dentro de la misma ventana**. El encabezado Dreamcast Recompiled y el reloj
  de Windows siguen visibles.
- «‹ Inicio» vuelve al menú de iconos sin crear más ventanas.
- La tira superior ofrece acceso directo entre Juegos, Vídeo, Audio,
  Rendimiento, Mandos, VMU y Ajustes.
- Juegos muestra CDI/ISO, disponibilidad, selección, ejecución y preparación
  opcional SH-4. Agregar o seleccionar un juego no recompila SH-4.
- Los cambios de vídeo, audio y rendimiento se guardan al seleccionarlos y
  se aplican en el siguiente inicio. Algunos filtros/controles avanzados son
  solo preferencias guardadas y están señalados como pendientes.

## Mandos

La pestaña integrada prueba el mando conectado por XInput (Xbox/Steam Input)
 o DirectInput/WinMM (incluidos controladores PS4/PS5 compatibles con WinMM),
 muestra botones y ejes y permite editar/guardar asignaciones reales en
 `profiles/controller_profile.ini`. Guarda copia `.ini.bak` de la versión anterior.

«Abrir Visual Tester completo» abre explícitamente
 `tools/controller_test/DreamcastControllerTest.ps1`, el probador visual de
 aprendizaje, ejes y gatillos. **Ya no se ejecuta por defecto el configurador
 antiguo** `DreamcastControllerConfig.ps1`.

## Aislamiento y compatibilidad

No se modificó ningún archivo ajeno a `launcher/`, ni los binarios de cpp38e,
 ni runtime DXGI de V8.4, ni su caché de SH-4, ni VSync. Los selectores de
 resolución de salida 960p, 1080p, 1440p y aspecto 16:9 siguen disponibles.
 El 16:9 actual estira la imagen; aún no es widescreen con más campo de visión.

En Windows, el explorador de archivos puede mostrar el diálogo normal del SO
 para elegir CDI/ISO; el Visual Tester completo abre su herramienta externa
 únicamente cuando se solicita. El resto de apartados utiliza la ventana principal.

Para abrir: `INICIAR_DREAMCAST_RECOMPILED_V1_2.bat`.
