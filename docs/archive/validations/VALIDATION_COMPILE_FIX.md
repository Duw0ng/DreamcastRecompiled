# Validación de la corrección de compilación (2026-09-15)

- Build Release de herramientas: PASS, GCC 13/Linux.
- CTest completo: 53/53 PASS. Incluye compilar/enlazar ambas salidas del emisor
  y ejecutar sus generated_compile_test.
- Se reprodujo el error original dc_guest_read32_hot no declarado en runtime.
- Fixture comercial RAW sintética: 300 funciones / 32 shards; 900 instrucciones
  conocidas, 0 desconocidas, RAW_SH4=0. Build Release y enlace completos: PASS.
- generated_compile_test de la fixture RAW: exit 0.
- dreamcast_program de la fixture RAW: exit 0, retorno PC=0xFFFFFFFF.
- Evidencias: carpeta validation_compile_fix.
- Pendiente en equipo Windows: MSVC, ejecución de BAT/PowerShell, D3D11/audio
  y CDI real de ChuChu. La fixture no representa una partida ni mide FPS.
