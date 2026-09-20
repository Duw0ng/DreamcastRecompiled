# DreamcastRecomp 0.0.172 — CT2 post-VMU closure

## Objetivo

Convertir los avances posteriores a 0.0.171 en una build manualmente testeable, sin auto-inputs ni watchpoints de diagnóstico.

## Cambios incorporados

- Mantiene los fixes generales validados en 0.0.171: BRAF local de 32 bits, ARM7TDMI `STM ... {pc}` con PC+12, staging de boot comercial, modem/G2 mínimo compatible, literales PC-relativos mutables y soporte del allocator guest provisional.
- La salida generada de Crazy Taxi 2 registra permanentemente el cierre dinámico descubierto después de aceptar el VMU.
- Añade la familia `0x8C0372A0` y sus dependencias limpias.
- Añade la familia `0x8C037790` y sus dependencias/bloques internos, incluyendo el flujo de confirmación de creación del save.
- Se eliminan todos los probes temporales que forzaban A/START/YES y los watchpoints usados para observar `PDS_PERIPHERAL`.
- Se restaura un VMU limpio en el paquete de prueba para que el flujo sea reproducible.

## Milestone observado durante desarrollo

Con los nuevos targets, Crazy Taxi 2 progresó desde `NOW LOADING` a la selección de VMU, el diálogo `No Save File`, la confirmación de creación del archivo de Crazy Taxi 2 y nuevas lecturas GD-ROM posteriores. Ese avance se obtuvo con probes de input temporales; esos probes NO forman parte de 0.0.172.

## Estado de prueba esperado para el usuario

La build de `experimental_ct2_0.0.172` debe compilar y permitir manejar manualmente el pad/VMU con teclado o XInput. La validación de 0.0.172 se considera exitosa si reproduce el boot comercial y alcanza la UI de VMU sin missing-target en las familias agregadas.

## Limitación conocida

La reconstrucción de estado de `syMallocInit` sigue siendo provisional y específica del camino comercial investigado. El allocator que se ejecuta después continúa siendo el código SH-4 recompilado del propio juego.

## Validación ejecutada antes del ZIP

- Build Release/Ninja del proyecto principal: PASS.
- CTest del proyecto principal: **50/50 PASS**.
- Compilación aislada de `generated_dd80.cpp` limpio: PASS.
- Enlace incremental de la salida comercial completa con los nuevos targets: PASS.
- `generated_compile_test`: **RC=0**.
- Escaneo del paquete: no quedan marcadores CT2 de auto-input (`CT2-UI-SYNC-A`, `CT2-CONFIRM-YES-A`, `CT2-YES-CONFIRM`) ni watchpoints `PAD-WRITE/MAP-WRITE/WATCH*`.
