# DreamcastRecomp cpp34v — FOURTH LATE-AOT ONLINE

- Extiende el overlay ABI1 de cpp34u con el nuevo target runtime `0x8C036236`.
- Conserva `0x8C036102`, `0x8C036118` y `0x8C036526` en la misma `dcr_guest_patch.dll`.
- No modifica `dcr_network.cpp`: mantiene cpp34s V.42/LAPM reliability + cpp34r fast path.
- No modifica `dc_runtime.hpp`, el emitter ni el guest principal de 5185 funciones.
- `TEST_CPP34V_LATE_AOT4_ONLINE.bat` migra el overlay solo si falta alguno de los cuatro targets.
- El target `0x8C036236` fue validado con los 16 bytes reportados por el runtime: 8/8 instrucciones SH-4 conocidas, `RAW_SH4=0`.
- Evidencia del nuevo milestone: conexión real al puerto `9001`, banner `DreamCast Lobby Server. Copyright SEGA Enterprises. 1999`, intercambio de payload posterior y llegada visual al menú online/estado Connected.
