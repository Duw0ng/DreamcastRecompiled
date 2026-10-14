# DreamcastRecomp cpp34x — AUTO Late-AOT ONLINE

- Añade el nuevo target runtime `0x8C036280` observado ya dentro del lobby real de ChuChu Rocket.
- Conserva `0x8C036102`, `0x8C036118`, `0x8C036160`, `0x8C036236` y `0x8C036526`.
- Nuevo modo **AUTO Late-AOT**: al volver a ejecutar el BAT, escanea los logs recientes del mismo paquete y agrega automáticamente nuevos errores `No recompiled/native target...` válidos al overlay.
- El auto-promoter solo acepta targets SH-4 pares dentro de P1 main RAM `0x8C010000..0x8CFFFFFF`.
- No modifica networking: conserva cpp34s V.42/LAPM reliability + cpp34r fast path.
- No modifica `dc_runtime.hpp`, emitter ni el guest principal de 5185 funciones.
- El target `0x8C036280` fue validado con los 16 bytes del runtime: 8/8 instrucciones SH-4 conocidas, `Unknown=0`, `RAW_SH4=0`.
- Antes del fallo, la sesión ya estaba conectada al Lobby Server `146.185.135.179:9001`, con `ppp19/ip14/tcp13`, y recibió otro payload de 156 bytes.
