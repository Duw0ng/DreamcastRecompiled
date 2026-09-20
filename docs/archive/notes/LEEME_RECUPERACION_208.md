# Recuperación parcial de Dreamcast Recompiled 0.0.208

Fuentes sin compilar. No es una copia íntegra certificada del entorno del otro chat.

## Archivos recuperados exactamente

Base original: DreamcastRecomp_0.0.207_DaytonaPressStart_20260918.zip.
SHA-256 comprobado: def620752f58f337486e70819bfaee45c7b310373fef5ef52859dd76d901db68.
Se extrajeron todos sus archivos y se conservaron sin modificaciones. Incluye fuentes,
headers, CMake, scripts, parches y evidencias históricas de la 0.0.207.
Las validaciones antiguas pertenecen a esa versión, no a esta reconstrucción.

## Cambios reconstruidos desde el historial

El chat «Probar contenedor», 18/09/2026, documentaba cuatro seeds posteriores:
0x8C044894, 0x8C05B2C8, 0x8C03F820 y 0x8C05B2D6.
Se añadieron a una COPIA del lanzador de Daytona, para un total de 25.
El lanzador original permanece intacto. El nuevo utiliza carpetas de salida propias.
No se modificó el motor C++ ni se renumeró la base como si fuera la 0.0.208 íntegra.

Archivos nuevos:
- run_daytona_recovery208.bat: lanzador reconstruido con 25 seeds.
- DAYTONA_RECOVERY208_SEEDS.txt: lista completa.
- RECUPERACION_208.patch: diferencia legible respecto al lanzador original.
- LEEME_RECUPERACION_208.md: procedencia y límites.

## Estado pendiente

El historial reportaba 3377 funciones, 268624 instrucciones, Unknown SH-4=0 y RAW_SH4=0,
y progreso después de LADIES & GENTLEMEN / START YOUR ENGINES. Son resultados
reportados en el otro chat, NO resultados reproducidos en esta recuperación.
No se encontró un ZIP, parche ni checkpoint íntegro guardado de la 0.0.208.
No se recuperaron posibles cambios adicionales no descritos en los mensajes.
No se compiló ni ejecutó este paquete, conforme al pedido de recuperar las fuentes.
No contiene el CDI de Daytona. No confirma llegada a parrilla, HUD o carrera jugable.

Para una futura prueba en Windows, con Visual Studio/CMake y tu CDI:

    run_daytona_recovery208.bat "C:\ruta\Daytona USA.cdi"

Ese comando sí compilará y ejecutará; no se ejecutó al preparar este paquete.
