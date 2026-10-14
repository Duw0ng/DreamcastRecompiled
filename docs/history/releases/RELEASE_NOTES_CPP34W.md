# DreamcastRecomp cpp34w — FIFTH LATE-AOT ONLINE

- Extiende el overlay ABI1 de cpp34v con el nuevo target runtime `0x8C036160`.
- Conserva `0x8C036102`, `0x8C036118`, `0x8C036236` y `0x8C036526` en la misma `dcr_guest_patch.dll`.
- No modifica `dcr_network.cpp`: mantiene cpp34s V.42/LAPM reliability + cpp34r fast path.
- No modifica `dc_runtime.hpp`, el emitter ni el guest principal de 5185 funciones.
- `TEST_CPP34W_LATE_AOT5_ONLINE.bat` migra el overlay solo si falta alguno de los cinco targets.
- El target `0x8C036160` fue validado con los 16 bytes reportados por el runtime: 8/8 instrucciones SH-4 conocidas, `RAW_SH4=0`.
- El bloqueo aparece ya dentro del lobby real, despues de recibir 156 bytes adicionales desde `146.185.135.179:9001`.
- Nota VMU: el aviso visual no impide el login/lobby; la cuenta remota funciona, pero el log actual no muestra escrituras/syncs locales a VMU ni programas de Flash.
