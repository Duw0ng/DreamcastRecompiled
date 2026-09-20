# DreamcastRecomp 0.0.208 - FIX1 (Daytona USA)

## Que corrige
Crash al terminar una carrera (tiempo agotado / fin de carrera):

    [DreamcastRecomp ERROR] No recompiled/native target registered for
    Dreamcast address 0x8C05B2EC (guest_pc=0x8C05A772 ...)

## Causa
0x8C05B2EC es una funcion real que el juego llama de forma indirecta desde una
tabla de handlers de estado en 0x8C19FB70. El recompilador solo descubre
destinos por literales dentro del codigo, asi que los punteros que viven en
tablas de datos no se registraban. Es el mismo patron de 0x8C05B2C8 y
0x8C05B2D6, que ya habia que agregar como seeds a mano.

## Cambio
Se agregan 29 seeds a los 25 existentes (54 en total). Se eligieron asi:
1. Se listaron 48 punteros de tablas hacia zonas de codigo (0x8C010000-0x8C07FFFF
   y 0x8C1B0000-0x8C1FFFFF) que no estaban registrados.
2. Cada uno se probo por separado con dc_raw_recomp --no-emit.
3. Se conservaron los 29 que decodifican sin instrucciones desconocidas.
   Se descartaron 19 que son datos (p.ej. 0x8C078AD8 mete ~1290 desconocidas).

Resultado del recompilador: 3412 funciones (antes 3377), Unknown SH-4 = 0,
RAW_SH4 = 0.

Archivos: run_daytona_fix1.bat (nuevo, 54 seeds, salida en generated\daytona_fix1
y _cb_daytona_fix1 para no mezclar objetos viejos), DAYTONA_FIX1_SEEDS.txt,
FIX1_cambios.diff. run_daytona_0.0.208_pruebas.bat ahora llama a FIX1.
run_daytona_recovery208.bat queda intacto.

Uso: igual que antes con run_daytona_0.0.208_pruebas.bat.

## Verificado (Linux, GCC, sin ventana, sin GPU ni audio)
- Recompilacion y compilacion del runner sin errores.
- Menus -> Single Race -> carrera con HUD, vuelta 1 completada, TIME EXTENSION.
- Tiempo agotado -> RESULT (GAME OVER) -> RACE END MENU sin el error anterior.
- RETRY reinicia la carrera. MAIN SELECT vuelve al ciclo de titulo/attract.

## NO verificado
- Las 4 vueltas completas: el limite de tiempo exige ~23 s por vuelta tras la
  vuelta 1 y mi piloto automatico hace ~29 s. Ninguna carrera llego a la meta.
- Ventana D3D11, controles reales, audio, fluidez y build en Windows.
- 0x8C066316 y 0x8C067C2A no agregaron funciones como seeds; no esta confirmado
  que esas dos entradas de la tabla queden resueltas.
- Pueden faltar otros destinos de tablas fuera de las zonas de codigo revisadas.
- Defecto visual conocido: bloque de pixeles corrupto donde va el nombre del auto
  en MAIN SELECT (no investigado).

## Si vuelve a fallar
Copia la linea "No recompiled/native target registered for ... address 0x8C......"
del log. Trae nearest-native y target-bytes. Si los bytes son codigo SH-4
valido, agregar esa direccion como --seed en run_daytona_fix1.bat.
