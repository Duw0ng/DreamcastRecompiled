# DreamcastRecomp cpp34u — THIRD LATE-AOT ONLINE

- Extiende el overlay ABI1 de cpp34t con el nuevo target runtime `0x8C036526`.
- Conserva `0x8C036102` y `0x8C036118` en la misma `dcr_guest_patch.dll`.
- No modifica `dcr_network.cpp`: mantiene cpp34s V.42/LAPM reliability + cpp34r fast path.
- No modifica `dc_runtime.hpp`, el emitter ni el guest principal de 5185 funciones.
- `TEST_CPP34U_LATE_AOT3_ONLINE.bat` migra el overlay solo si falta alguno de los tres targets.
- El target `0x8C036526` fue validado con los 16 bytes reportados por el runtime: 8/8 instrucciones SH-4 conocidas, `RAW_SH4=0`.
