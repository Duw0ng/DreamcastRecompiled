# Dreamcast Recompiled v1.2 — Intérprete SH-4 de respaldo (interp-fallback-v1)

Base: `SGGG_QUIET_REBIND_CACHE_FIX`, guest ABI v1 / `cpp38e`. Runtime: `cpp38e-v8.6-sh4-interp-fallback-v1`.

## Qué cambió

- **Un objetivo SH-4 sin código AOT ya no cierra el juego.** `call_recompiled()` lo ejecuta con un intérprete SH-4 nuevo (`runtime/dc_sh4_interp.cpp`) que respeta el contrato del código generado: RTS/RTE, ticks por bloque, interrupciones y `sh4_async_redirect`, y vuelta inmediata a AOT cuando llama o salta a código compilado.
- **Aprendizaje automático de semillas.** Cada entrada nueva cuyos bytes coinciden con el `BOOTSTRAP.BIN` se agrega a `<caché del juego>\interp_seeds.txt`. En la próxima "Preparar SH-4", `run_game.bat` la pasa al recompilador con `--seed-file`. El código que solo existe en tiempo de ejecución (overlays) se anota como `# runtime-code` y sigue interpretado.
- **Instrucciones que el AOT todavía no decodifica:** `MAC.L`, `MAC.W`, las formas `@(disp,GBR)` y `@(R0,GBR)`, `SLEEP` y `TRAPA` funcionan en código interpretado.
- **Opciones nuevas:** `--no-interp` (comportamiento anterior) y `--interp-trace`. En el launcher: Diagnóstico → Intérprete SH-4.
- **Sin recompilar juegos.** La Guest ABI1 y sus headers no cambiaron. El nuevo `DCR_RUNTIME_BUILD_ID` hace que Windows reconstruya solo el runtime y los plugins.
- Los tests de contrato del launcher que comparaban el ID de runtime exacto ahora aceptan cualquier `cpp38e-v8.x`: suite del launcher 130/130.

## Probar en Windows

```bat
run_game.bat "C:\ruta\SGGG.cdi" --reuse --no-portable-sync
run_game.bat "C:\ruta\SGGG.cdi" --reuse --no-portable-sync --interp-trace
run_tests.bat
```

Si un juego llegaba hasta `No recompiled/native target registered...`, ahora debería seguir. Las líneas `[DCR SH4-INTERP] new entry ...` muestran qué se está interpretando.

## Validación (Linux)

- `dcr_sh4_interp_tests`: 271/271 (corpus C diferencial SH-4 contra host, modo mixto, delay slots, GBR, MAC, IRQ a mitad de función, SLEEP, TRAPA, semillas).
- `tools/interp/e2e_aot_interp_check.py`: pipeline real; aborta con `--no-interp`, resultados bit-exactos con el intérprete y 0 entradas interpretadas tras volver a sembrar.
- `dcr_portable_ipbin_stage_tests`: 1/1. Launcher bajo Xvfb: 130/130.
- **Pendiente:** compilación con MSVC y FPS reales en Windows. No se declara ninguna mejora de compatibilidad por título.

Detalle técnico: `docs/SH4_INTERPRETER_FALLBACK_v1.2.md`.
