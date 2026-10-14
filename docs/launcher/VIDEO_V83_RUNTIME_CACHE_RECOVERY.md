# Dreamcast Recompiled v1.2 — V8.3 runtime cache recovery

## Causa comprobada

El BAT de V8.2 seguía utilizando `DCR_RUNTIME_BUILD_ID=cpp38e-hot-block-recovery`,
el mismo sello que la versión anterior. Por eso `--reuse` podía iniciar un
`DreamcastRuntime.dll` antiguo aun cuando el código fuente nuevo estuviese
presente en el ZIP.

La evidencia de la ejecución anterior es el mensaje:

```
[PVR GPU] DXGI scanout ready (1280x960, Present sync=0, output stretch=4:3)
```

El código de V8.2 ya agregaba `video-scaler=v8.2/no-cull`; esa terminación
nunca apareció en los registros aportados. También faltaba el aviso de la
salida GDI en los registros examinados. La regresión de imagen **todavía debe
reprobarse en Windows con la DLL correcta**; estos logs no demuestran que el
shader de V8.2 tuviera un fallo nuevo después de compilarse.

## Cambios

1. `run_game.bat` separa la versión de los binarios del runtime (ahora
   `cpp38e-v8.3-video-output-recovery`) de la revisión **inmutable del guest**
   (`cpp38e-hot-block-recovery-v1`).
2. Cada inicio compara una huella SHA-256 de fuentes `runtime/` y los cuatro
   binarios compilados contra un manifiesto guardado al terminar el build.
   El sello de texto por sí solo ya no basta para considerar válida la DLL.
3. Si el manifiesto falta o no coincide, configura/compila de nuevo **solo el
   runtime** con una limpieza del árbol CMake separado. No recompila ni enlaza
   `dcr_game_module.dll`, no redescubre funciones SH-4, no borra el CDI.
4. La inspección `CMAKE_HOME_DIRECTORY` utiliza Python en lugar del antiguo
   PowerShell con tuberías escapadas (`^|`) que provocaba errores en Windows.
5. El runtime imprime `video-scaler=v8.3` al abrir el escáner DXGI y
   `presentation=v8.3/GDI` cuando se elige compatibilidad GDI.
6. Se conserva `Present(0u, 0u)`; **VSync no se cambia**.

## Comprobación en Windows

Extraer el ZIP completo en una carpeta nueva. Ejecutar la misma acción
«Jugar» del launcher con ChuChu Rocket! y resolución de 960p. La primera
vez debería mostrar `RUNTIME VERIFY` y la reconstrucción del runtime. No
se debe mostrar una recompilación de `dcr_game_module.dll`.

En el log del juego debe aparecer alguno de los siguientes mensajes:

- DX11: `[PVR GPU] DXGI scanout ready (... video-scaler=v8.3/no-cull)`.
- GDI: `[VIDEO OUTPUT] presentation=v8.3/GDI ...`.

Si no aparece ninguno, comprobar que el juego se inició desde **la carpeta
V8.3** y enviar el log de la consola y el de la sesión. Los perfiles por juego
pueden reemplazar el backend seleccionado globalmente en Ajustes > Vídeo.

## Límites de validación

Se han ejecutado pruebas automatizadas del launcher y del verificador, además
de compilar el runtime en Linux. No se ha podido ejecutar Direct3D 11 en
Windows desde este entorno, por lo que no se certifica todavía que
960p/1080p se vean correctamente en la RX 580.
