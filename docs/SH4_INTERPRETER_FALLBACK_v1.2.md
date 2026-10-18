# SH-4 Interpreter Fallback (interp-fallback-v1)

Baseline: v1.2 cpp38e HOT_BLOCK_RECOVERY · Frozen Guest ABI1 · runtime `cpp38e-v8.6-sh4-interp-fallback-v1`

## Problem addressed

The AOT recompiler only knows SH-4 entry points discovered during static closure analysis. Previously, any dynamic call or branch to an uncompiled address (function pointers, callback tables, code loaded from disc, or overlays) ended with:

```text
No recompiled/native target registered for Dreamcast address 0x........
```

The game then terminated. Consequently, each title required manual seed lists (`profiles/*_known_seeds.txt`) and tuning of closure limits.

The interpreter **executes the missing address directly from guest RAM**, and records it so a later **Prepare SH-4** operation can compile it natively.

## How it works

1. `call_recompiled()` resolves the target using the normal dispatch cache, P1/P2 aliases, and relocated-code templates.
2. If no compiled function exists and the address lies in guest main RAM, it invokes `dc_sh4_interp_function` instead of throwing. The interpreter uses the same signature as generated native functions and is entered into the `dispatch_cache` just like AOT code.
3. The interpreter honors the generated-code calling contract:
   - `RTS` sets `ctx.pc = PR`; `RTE` sets `ctx.pc = SPC`.
   - Every block boundary calls `dc_runtime_tick_fast()`. Interrupts and `sh4_async_redirect` unwind the call stack exactly as they do in AOT.
   - If interpreted code branches or calls a compiled address, it **returns immediately to native execution**. AOT code may also call interpreted functions.
4. When an entry is first interpreted, its current RAM bytes are compared with `BOOTSTRAP.BIN` (the exact input used by the recompiler):
   - **Bytes match:** record it as an active seed in `interp_seeds.txt`.
   - **Bytes differ or fall outside the image** (runtime overlays or generated/copied code): label the entry `# runtime-code` and do not seed it, because the static recompiler would read different bytes at that address. It remains interpreted.
   - Resume addresses (after an interrupt, a return from interpreted code, or `SLEEP` / `TRAPA`) are never recorded as seeds.

**Guest ABI1 remains unchanged.** The layouts of `DCRuntime` and `SH4Context` and the headers under `runtime/guest_abi/v1` were not modified for this feature. Interpreter-specific state is stored in a runtime side table. Already-compiled guest modules remain valid; only the runtime must be rebuilt. `run_game.bat` does so when the `DCR_RUNTIME_BUILD_ID` changes.

## Usage

The interpreter is enabled by default, so no extra action is normally necessary.

```bat
rem Normal: interpret and learn entries without AOT coverage
run_game.bat "C:\path\game.cdi" --reuse

rem Trace each newly interpreted entry
run_game.bat "C:\path\game.cdi" --reuse --interp-trace

rem Previous behavior: abort at an address without an AOT target
run_game.bat "C:\path\game.cdi" --reuse --no-interp
```

Launcher path: **Diagnostics → SH-4 Interpreter → Fallback SH-4 Interpreter / SH-4 Interpreter Trace**.

Learned seeds are stored in `%LOCALAPPDATA%\DreamcastRecompiled\games\<GAME>\interp_seeds.txt`. The runner reports the pending count at startup. During the next SH-4 preparation (Games → Prepare SH-4, or a run without `--reuse`), they are passed to the recompiler as `--seed-file`.

Runtime variables: `DCR_SH4_INTERP=0|1`, `DCR_SH4_INTERP_SEEDS=<file>`, and `DCR_SH4_INTERP_TRACE=0|1`.

## Instruction coverage

The interpreter implements the SH-4 instructions already used by AOT with matching semantics. It shares runtime helpers for memory, store queues, FR/XF banks, `FPSCR.SZ/PR/FR`, `dc_write_sr`, `dc_write_fpscr`, and `dc_unimplemented`. It also handles instructions that the static decoder **does not yet recognize**:

| Instruction | AOT | Interpreter |
| --- | --- | --- |
| `MAC.L @Rm+,@Rn+` / `MAC.W @Rm+,@Rn+` (S-bit saturation) | No | Yes |
| `MOV.B/W/L R0,@(disp,GBR)`, `MOV.B/W/L @(disp,GBR),R0` | No | Yes |
| `TST.B/AND.B/XOR.B/OR.B #imm,@(R0,GBR)` | No | Yes |
| `SLEEP` (wait for next interrupt) | No | Yes |
| `TRAPA #imm` (TRA/EXPEVT, VBR+0x100) | No | Yes |
| `LDTLB` | No | No (same limitation as AOT) |

The recompiler discards functions containing unknown instructions. Those functions can now run through the interpreter instead of terminating the game, but remain interpreted until the decoder/DCIR adds their instruction support.

## Original validation

- `runtime/tests_sh4_interp.cpp` (CTest `dcr_sh4_interp_tests`, 271 assertions):
  - **Differential tests:** the same real C corpus (`tests/interp/interp_corpus.c`), cross-compiled with GCC for SH-4 (`-ml -m4`), runs through the interpreter and must match a native host build. It covers arithmetic, libgcc division, 64-bit calculations, jump tables, recursion, function pointers, bitfields, floating-point values, FPSCR.PR changes, and conversions.
  - **Mixed execution:** functions registered as AOT test interpreted-to-AOT calls, AOT-to-interpreted calls, and tail branches to AOT.
  - **Assembly probes** (`tests/interp/interp_asm.S`): delay slots (including `BT/S` with T modified in the slot, `RTS` / `BSR`), GBR forms, saturated and unsaturated `MAC.W` / `MAC.L`, mid-function interrupts with unwind/resume, `SLEEP`, and `TRAPA`.
  - Seed-file checks: verified entries, `runtime-code`, continuation exclusion, and cross-session deduplication.
  - `DCR_SH4_INTERP=0` and targets outside guest main RAM retain the historical failure.
- `tools/interp/e2e_aot_interp_check.py` runs the full pipeline (`dc_raw_recomp` → modular guest → runtime). Without the interpreter the program aborts at the historical error. With the interpreter, six function-pointer-only entries execute with results identical to the host. After native recompilation using the learned seeds, those entries no longer require interpretation.
- Instruction-by-instruction comparison with `qemu-sh4` agrees except for known QEMU 8.2 issues involving `ADDV` / `SUBV` / `ROTL` / `ROTR` and `DIV1` with a zero divisor.
- Linux performance was measured at approximately **120 MIPS** using `--sh4-tick-batch=256`, comparable to the effective rate of a real SH-4 for cold paths. Frequently executed paths can move to AOT at the next preparation step.

**Windows/MSVC remained to be tested in the original report:** build the runtime, run `run_tests.bat`, and test real games.

### Regenerating SH-4 fixtures

`tests/interp/interp_fixtures.inc` is versioned, so Windows does not need a cross-compiler. To regenerate it on Linux:

```sh
sudo apt install gcc-sh4-linux-gnu
python3 tools/interp/build_interp_fixtures.py
```

## Additional finding

For `DIV1 Rn,Rn` (the same register as divisor and dividend), Rm must be read **after** Rn is shifted. The interpreter does this correctly, whereas the original AOT implementation (`cpp_emitter.cpp`, `DCIROp::Div1`) reads the previous value. Compilers do not normally generate this form, so known games were not affected, but the emitter should eventually be aligned.

## Known limitations

- The older embedded, non-modular runtime in `cpp_emitter.cpp` does not include this interpreter. Commercial game paths use the modular `runtime/` implementation, which does.
- A corrupted function pointer inside guest main RAM may execute arbitrary bytes until an illegal instruction is reached. The resulting error reports both the interpreted entry and its caller. `--no-interp` restores immediate failure on an unsupported target.
- PC-relative instructions in a delay slot use the slot's own address, as they do in AOT. Occurrences are counted in `pc_relative_in_delay_slot` diagnostics.
