# Límites de funciones SH-4 por juego — Dreamcast Recompiled v1.2

Esta revisión toma como base `TESTER_DEBUG_MENU_WINSOCK_FIXED` y no modifica SH-4 runtime, plugins, Winsock ni las implementaciones de render.

## Problema observado

SGGG tiene una referencia histórica de 6.393 funciones SH-4 alcanzables. El valor **6.144** en los perfiles históricos **no era el máximo final de funciones**: era `max_closure_entries`, el presupuesto de entradas de descubrimiento. El valor total `max_functions` era 8.192.

Un presupuesto de closure de 6.144 no impide necesariamente observar 6.393 funciones al final: pueden aparecer funciones adicionales durante el análisis final. Subir ese presupuesto permite explorar más posibles entradas, pero no garantiza llegar a más funciones correctas ni un mejor arranque. Vigilar `Unknown SH-4`, `RAW_SH4`, targets rechazados y la calidad del CFG.

## Nuevos valores predeterminados

| Juego | max_functions | max_closure_entries | closure_passes |
|---|---:|---:|---:|
| SGGG | 10240 | 7680 | 64 |
| ChuChu Rocket! | 8192 | 6144 | 64 |
| Crazy Taxi 2 | 8192 | 6144 | 64 |
| Daytona USA | 8192 | 6144 | 64 |
| Record of Lodoss War | 10000 | 9000 | 64 |
| Títulos desconocidos | 8192 | 6144 | 64 |

**Importante:** los perfiles persistentes de cada juego son independientes. La migración automática de SGGG opera solo cuando encuentra el perfil histórico v1 con la pareja de valores 8192/6144. Los perfiles personalizados no se sobrescriben.

## Configuración desde el launcher

1. Abrí `ABRIR_LAUNCHER.bat` y seleccioná **Perfiles por juego** en el panel avanzado.
2. Elegí el juego cuya configuración querés ampliar.
3. Modificá `Máximo de funciones finales`, `Entradas de closure`, o `Pasadas de análisis`.
4. Para SGGG, podés pulsar **Valores recomendados SGGG** y después **Guardar límites**.
5. Usá **Juegos → Preparar SH-4** (o `run_game.bat "juego.cdi"` sin `--reuse`) para recompilar el guest de ese título. El botón habitual **Jugar** utiliza `--reuse`, por lo que se detendrá si detecta límites pendientes; no recompila automáticamente. Las cachés de los demás juegos no se tocan.

También podés editar `%LOCALAPPDATA%\DreamcastRecompiled\games\<JUEGO>\game.cfg` manualmente. Si editás el archivo a mano, ejecutá `run_game.bat "ruta\juego.cdi" --recompile-game` para aplicar las modificaciones, dado que la edición externa no crea el marcador del launcher.

`--reuse` nunca debe recompilar. Si hay un cambio de límites pendiente, rechaza el módulo antiguo con RC=14 y explica que hace falta iniciar normalmente.

## Telemetría a revisar

En la salida del recompilador buscá:

- `Closure entry budget: 7680 / 10240` para SGGG.
- `Synthetic entries` y `Reachable functions`, que **no tienen obligación de ser iguales**; son conceptos distintos.
- `Unknown SH-4: 0` y `RAW_SH4: 0` como criterios de calidad de las instrucciones que se han incluido.
- `Closure cap hit`: si aparece, puede haber descubrimiento limitado por presupuesto.

No se puede confirmar que SGGG arranque hasta compilar la revisión en Windows y probar el juego.

## Validación hecha antes del empaquetado

- Los 118 tests del launcher Python aprobaron en Linux con Xvfb.
- El editor gráfico se abrió y una prueba guardó correctamente 10240/7680/64 sobre un perfil SGGG simulado.
- El archivo de configuración original mantiene sus parámetros no relacionados y solo se modifica el perfil seleccionado.
- **No se ejecutó PowerShell de Windows ni una recompilación real de SGGG/CDI con MSVC aquí.**
