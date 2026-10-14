# Dreamcast Recompiled v1.2 — Consola y heartbeats opcionales

Desde **Ajustes > Diagnóstico**:

- **Mostrar consola externa al jugar**: desactivado de fábrica. Con la opción desactivada, el launcher lanza el BAT y las herramientas mediante `CREATE_NO_WINDOW` y conserva la salida del proceso en `%LOCALAPPDATA%\DreamcastRecompiled\launcher_v1_2\launcher_console.log` (o en la carpeta de configuración que corresponda).
- **Heartbeats periódicos (1 s)**: desactivado de fábrica. `run_game.bat` envía `--diag-heartbeat-ms=0` al guest. Al activarlo usa `--heartbeat-on` -> `--diag-heartbeat-ms=1000`.

Debug avanzado y heartbeats son independientes; otros scripts históricos de pruebas aún podrían elegir explícitamente heartbeats. La ruta estable desde el launcher no recompila el guest SH-4.

En la consola oculta se guarda igualmente el contenido que antes hubiera aparecido en el terminal. La captura no sustituye a los logs de sesión del runtime.

Estas opciones se aplican **al próximo arranque del juego**, sin cerrar el launcher, y ambas quedan persistidas en settings.json.
