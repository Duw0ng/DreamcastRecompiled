# DreamcastRecomp 0.0.105 — validación

- 47/47 CTest PASS.
- `cpp_emitter_tests` verifica mediana-5, umbral 2048, cuatro pasadas y continuidad cross-sector del resampler YB.
- Runner recién emitido compila y enlaza (`dc_runtime.cpp`, `generated_program.cpp`, `generated_compile_test`, `dreamcast_program`).
- El cambio se limita al pipeline CDDA `yb`; las pistas `raw` no pasan por el filtro.
