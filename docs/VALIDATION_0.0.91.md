# DreamcastRecomp 0.0.91 validation

## Motivo

0.0.90 elimino los underruns de WinMM, pero el log real mostro `arm7-insn` muy bajo y `host-aica-drop` enorme. El cap de 256 pasos por host sync descartaba elapsed AICA clocks, ralentizando timers/FIQ y la secuencia musical aunque el PCM de 44.1 kHz siguiera continuo.

## Cambio

- Host PCM sigue a 44.1 kHz.
- Timer-idle AICA se fast-forwardea hasta el proximo IRQ habilitado.
- IRQ/FIQ y comandos SH-4 vuelven al interprete ARM7 con budget 4096.
- Telemetria nueva: `host-aica-ff=steps/events/active`.

## Tests

- CTest: 47/47 PASS.
- ChuChu closure: 2965 funciones; 295215 SH-4 conocidas; 0 unknown; RAW_SH4=0.
- Generated `dc_runtime.cpp` y `dc_arm7.cpp`: GCC C++20 compile PASS.
- Generated runner: C++20 syntax PASS.
- Generated commercial program: no se completa la validacion integral dentro del timeout del contenedor; la closure/codigo SH-4 no cambia por este fix de runtime.
- Synthetic Timer A fast-forward: 1024 clocks -> SCIPD bit 6 + FIQ asserted PASS.

## Windows A/B esperado

Comparar 0.0.90 vs 0.0.91 en title/gameplay. En 0.0.91 se espera que `host-aica-ff` crezca rapidamente, `host-aica-drop` deje de crecer de forma masiva, `native-starts`/slot changes avancen con cadencia normal y `audio-starves` permanezca cercano a cero.
