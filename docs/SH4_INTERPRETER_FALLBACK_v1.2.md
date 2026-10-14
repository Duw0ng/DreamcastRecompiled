# Intérprete SH-4 de respaldo (interp-fallback-v1)

Base: v1.2 cpp38e HOT_BLOCK_RECOVERY · Guest ABI1 congelada · runtime `cpp38e-v8.6-sh4-interp-fallback-v1`

## Qué problema resuelve

El recompilador AOT solo conoce las entradas SH-4 que su closure estática encontró. Hasta ahora, cualquier salto o llamada dinámica a una dirección no compilada (un puntero a función, una tabla de callbacks, código cargado desde el disco, un overlay) terminaba en:

```
No recompiled/native target registered for Dreamcast address 0x........
```

y el juego se cerraba. Por eso cada título necesitaba semillas manuales (`profiles/*_known_seeds.txt`) y ajustes de límites de closure.

Con el intérprete, esa dirección **se ejecuta igual**, directamente desde la memoria del guest, y queda registrada para que la próxima "Preparar SH-4" la compile de forma nativa.

## Cómo funciona

1. `call_recompiled()` busca el objetivo como siempre: caché de despacho, alias P1/P2, plantillas de código reubicado.
2. Si no existe ninguna función compilada para ese objetivo y la dirección está en la RAM principal del guest, en vez de lanzar la excepción usa `dc_sh4_interp_function`. Esa función tiene la misma firma que las funciones recompiladas y queda guardada en la `dispatch_cache` igual que el código AOT.
3. El intérprete respeta el mismo contrato que el código generado:
   - `RTS` deja `ctx.pc = PR`; `RTE` deja `ctx.pc = SPC`.
   - En cada borde de bloque llama a `dc_runtime_tick_fast()`. Las interrupciones y `sh4_async_redirect` desenrollan la pila exactamente como en el AOT.
   - Si el código interpretado llama o salta a una dirección compilada, **vuelve inmediatamente a código nativo**. El código AOT también puede llamar a código interpretado.
4. La primera vez que se interpreta una entrada, el runtime compara sus bytes en RAM con el `BOOTSTRAP.BIN` (la imagen exacta que lee el recompilador):
   - **Coinciden:** se agrega como semilla activa a `interp_seeds.txt`.
   - **No coinciden o están fuera de la imagen** (overlays, código generado o copiado en tiempo de ejecución): se anota como `# runtime-code`. No se usa como semilla, porque el recompilador leería otros bytes en esa dirección. Ese código sigue interpretado, que es lo correcto.
   - Las direcciones de reanudación (retorno tras una interrupción, retorno de una llamada hecha desde código interpretado, después de `SLEEP` o `TRAPA`) nunca se guardan como semillas.

La Guest ABI1 no cambió: no se tocaron el layout de `DCRuntime` ni el de `SH4Context`, ni los headers de `runtime/guest_abi/v1`. El estado propio del intérprete vive en una tabla lateral del runtime. Los guests ya compilados siguen siendo válidos; alcanza con reconstruir el runtime, y `run_game.bat` lo hace solo al detectar el nuevo `DCR_RUNTIME_BUILD_ID`.

## Uso

No requiere ninguna acción: viene activo por defecto.

```bat
rem Normal: las entradas sin AOT se interpretan y se aprenden
run_game.bat "C:\ruta\juego.cdi" --reuse

rem Registrar cada entrada nueva interpretada
run_game.bat "C:\ruta\juego.cdi" --reuse --interp-trace

rem Comportamiento anterior (abortar ante un objetivo sin AOT)
run_game.bat "C:\ruta\juego.cdi" --reuse --no-interp
```

En el launcher: **Diagnóstico → Intérprete SH-4 → Intérprete SH-4 de respaldo / Traza del intérprete SH-4**.

Las semillas aprendidas quedan en `%LOCALAPPDATA%\DreamcastRecompiled\games\<JUEGO>\interp_seeds.txt`. Al iniciar, `run_game.bat` muestra cuántas hay pendientes. En la próxima preparación SH-4 (Games → Preparar SH-4, o una ejecución sin `--reuse`) se pasan automáticamente al recompilador como `--seed-file`.

Variables del runtime: `DCR_SH4_INTERP=0|1`, `DCR_SH4_INTERP_SEEDS=<archivo>`, `DCR_SH4_INTERP_TRACE=0|1`.

## Cobertura de instrucciones

El intérprete implementa el set SH-4 que usa el AOT, con la misma semántica: usa los mismos helpers del runtime para memoria, store queues, bancos FR/XF, `FPSCR.SZ/PR/FR`, `dc_write_sr`, `dc_write_fpscr` y los mismos `dc_unimplemented` para los casos no soportados. Además implementa instrucciones que el decodificador estático **todavía no conoce**:

| Instrucción | AOT | Intérprete |
| --- | --- | --- |
| `MAC.L @Rm+,@Rn+` / `MAC.W @Rm+,@Rn+` (con saturación por bit S) | no | sí |
| `MOV.B/W/L R0,@(disp,GBR)`, `MOV.B/W/L @(disp,GBR),R0` | no | sí |
| `TST.B/AND.B/XOR.B/OR.B #imm,@(R0,GBR)` | no | sí |
| `SLEEP` (espera la siguiente interrupción) | no | sí |
| `TRAPA #imm` (TRA/EXPEVT, VBR+0x100) | no | sí |
| `LDTLB` | no | no (mismo error que el AOT) |

Importante: el recompilador descarta las funciones que contienen instrucciones que no sabe decodificar. Esas funciones ahora se ejecutan interpretadas en vez de cerrar el juego, pero seguirán interpretadas hasta que el decodificador/DCIR aprenda esas instrucciones (siguiente paso natural).

## Validación

- `runtime/tests_sh4_interp.cpp` (CTest `dcr_sh4_interp_tests`, 271 comprobaciones):
  - **Diferencial:** un corpus en C real (`tests/interp/interp_corpus.c`), compilado con GCC para SH-4 (`-ml -m4`), se ejecuta solo con el intérprete y debe dar exactamente lo mismo que el mismo C compilado para el host. Cubre aritmética, división de libgcc, 64 bits, tablas de salto, recursión, punteros a función, bitfields, float y double con cambios de `FPSCR.PR`, y conversiones.
  - **Modo mixto:** funciones registradas como "AOT" con el contrato del código generado. Se prueban llamadas interpretado→AOT, AOT→interpretado y saltos de cola hacia AOT.
  - **Sondas en ensamblador** (`tests/interp/interp_asm.S`): delay slots (`BT/S` con T modificado en el slot, `RTS`/`BSR` con slot), formas GBR, `MAC.W`/`MAC.L` con y sin saturación, una interrupción en medio de una función (desenrollado y reanudación), `SLEEP` y `TRAPA`.
  - Archivo de semillas: entradas verificadas, `runtime-code`, exclusión de continuaciones y deduplicación entre ejecuciones.
  - Con `DCR_SH4_INTERP=0`, y para objetivos fuera de la RAM principal, se conserva el error histórico.
- `tools/interp/e2e_aot_interp_check.py`: prueba de punta a punta con el pipeline real (`dc_raw_recomp` → guest modular → runtime). Sin intérprete, el programa aborta con el error histórico. Con intérprete, las 6 entradas alcanzables solo por punteros a función se interpretan con resultados idénticos al host. Tras recompilar con las semillas aprendidas, la ejecución ya no interpreta nada.
- Contraste instrucción por instrucción con `qemu-sh4`: coinciden todas, salvo casos donde qemu 8.2 tiene errores conocidos (`ADDV`/`SUBV`/`ROTL`/`ROTR`) y `DIV1` con divisor cero.
- Rendimiento medido en Linux: unos 120 MIPS con `--sh4-tick-batch=256`. Es comparable a la tasa efectiva de un SH-4 real y suficiente para código frío. El código caliente pasa a AOT tras la siguiente preparación.

**Pendiente en Windows/MSVC:** compilar el runtime, correr `run_tests.bat` y probar con juegos reales.

### Regenerar los fixtures SH-4

`tests/interp/interp_fixtures.inc` está versionado, así que Windows no necesita compilador cruzado. Para regenerarlo (Linux):

```sh
sudo apt install gcc-sh4-linux-gnu
python3 tools/interp/build_interp_fixtures.py
```

## Hallazgo lateral

`DIV1 Rn,Rn` (mismo registro como divisor y dividendo) debe leer Rm *después* de desplazar Rn. El intérprete lo hace así; la versión AOT (`cpp_emitter.cpp`, `DCIROp::Div1`) lee el valor anterior. Los compiladores no generan esa forma, así que no afecta a juegos conocidos, pero conviene alinearlo en la próxima revisión del emisor.

## Limitaciones conocidas

- El runtime heredado embebido en `cpp_emitter.cpp` (modo no modular) no incluye el intérprete. Los juegos comerciales usan el runtime modular (`runtime/`), que sí lo incluye.
- Si un puntero corrupto apunta dentro de la RAM principal, se ejecuta lo que haya allí hasta encontrar una instrucción ilegal. El error indica la entrada interpretada y desde dónde se llegó. `--no-interp` restaura el corte inmediato.
- Las instrucciones PC-relativas dentro de un delay slot usan la dirección del slot, igual que el AOT. Se cuentan en las estadísticas (`pc_relative_in_delay_slot`) para detectarlas si aparecieran.
