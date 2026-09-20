# Dreamcast Recompiled 0.0.208 — versión de pruebas

Paquete experimental reconstruido desde las fuentes guardadas de 0.0.207.
Incluye los cuatro destinos SH-4 posteriores documentados en el otro chat.
No es una recuperación íntegra certificada de todos sus archivos temporales.

## Cómo probar en Windows

1. Extraer la carpeta DCR208_Test en una ruta corta, por ejemplo C:\DCR208_Test.
2. Tener Visual Studio con herramientas de desarrollo C++ y CMake.
3. Abrir una consola de herramientas de Visual Studio y ejecutar:

   run_daytona_0.0.208_pruebas.bat "C:\Juegos\Daytona USA.cdi"

El lanzador extrae tu disco, recompila el código SH-4, compila el runner y abre
la ventana del juego. La primera compilación puede tardar. El ZIP contiene
fuentes y scripts; no contiene el juego, su C++ generado ni un ejecutable Windows.
El motor conserva sus identificadores de 0.0.207: 0.0.208 identifica este paquete
de pruebas y su ampliación de destinos, no una validación nueva del motor.

Controles del perfil heredado:
Flechas = dirección; Enter = Start; Z/Espacio/J = A; X/K = B;
C/U = X; V/I = Y. Dar foco a la ventana PVR antes de usar el teclado.

## Qué incluye/parchea

Heredado de 0.0.205–0.0.207:
- Manejo de las regiones de módem y G2 externo sin dispositivo: lecturas cero
  y escrituras ignoradas, evitando los fallos en 0xA0600004 y 0xA1000400.
- Arranque comercial directo en 0x8C010000 con la preparación BIOS/GD-ROM HLE,
  para evitar la transformación duplicada del bootstrap de este disco.
- Perfil con reloj de dispositivos determinista para progresar en el arranque.
- Destinos SH-4 tardíos incorporados hasta PRESS START y la demostración 3D.

Reconstruido para esta prueba:
- Añadidos 0x8C044894, 0x8C05B2C8, 0x8C03F820 y 0x8C05B2D6 al perfil Daytona.
- El lanzador proporciona 25 seeds en total para incluir esos destinos durante
  la recompilación. Es una ampliación explícita de entradas, no una solución
  general que garantice descubrir todos los callbacks de cualquier juego.
- Carpetas de generación y compilación separadas para este perfil.

## Hasta dónde llegaba y qué se comprobó

0.0.207: los registros incluidos documentan CRI/ADX, Genki/WAVE MASTER,
Memory Card, confirmación sin guardar, NOW LOADING, PRESS START y demo 3D animada.
Las capturas y registros de validación incluidos pertenecen a ESA versión.

Avance posterior del otro chat: reportado hasta LADIES & GENTLEMEN / START YOUR
ENGINES y carga posterior. Reportó 25 seeds, 3377 funciones y cero RAW_SH4.
Es información recuperada del historial, no una nueva prueba de este paquete.
No está confirmada una carrera controlable, con parrilla/HUD y estabilidad.

En este chat: compilaron dc_disc_probe, dc_boot_prepare y dc_raw_recomp con GCC.
No se pudo repetir Daytona porque recuperar el RAR falló con HTTP 502.
No se ha validado este lanzador en Windows ni el runner con los 25 seeds.

Las notas antiguas 0.0.205/206 dudaban de la identidad del disco por su cabecera
PIZZICATO POLKA. La documentación 0.0.207 posterior corrige esa interpretación:
el contenido y la pantalla de título corresponden a Daytona USA.

Consultar LEEME_RECUPERACION_208.md y RECUPERACION_208.patch para la procedencia
exacta de los cambios. No se fusionó el experimento independiente 0.0.204 de
memoria inicial alternativa con esta rama posterior de arranque directo.
