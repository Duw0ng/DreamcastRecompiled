# DreamcastRecomp 0.0.22 — real Dreamcast/KallistiOS validation

0.0.22 continues the CPU-first phase, but measures progress primarily with native execution of real ELF code rather than only decoder coverage.

## 1. Noreturn CFG/data classification

The function analyzer now performs a second reachability pass after resolving calls. Calls to conservative, well-known noreturn C/newlib entries (`abort`, `exit`, `_Exit`, assert failures, stack-check failure) do not create a false successor after the delay slot.

This prevents literal pools, address tables and jump-table words placed after termination calls from being reported as executable SH-4.

Corpus effect on the supplied 155 ELF files:

- 155/155 `_main` reachable graphs remain at `RAW_SH4=0`.
- broad all-`STT_FUNC` unknown words drop from 73 to 3.
- 154/155 ELF files are now ISA-clean under the broad all-function scan.
- the final 3 words are all in Lua `_llex` after a compiler-generated cold helper (`_esccheck.part.0`) whose interprocedural noreturn property is not yet inferred; they are outside every `_main` RAW path.

The lower total 'known instructions' number is expected because false post-noreturn fallthrough is no longer counted as reachable code.

## 2. New real-fixture runner support

Generated native runners now accept:

- `--membin=ADDRESS:PATH` — load an arbitrary binary file into guest Dreamcast RAM.
- `--mem8=ADDRESS:VALUE`
- `--mem16=ADDRESS:VALUE`
- existing `--mem32`, `--memstr`, `--peek8/16/32` remain available.

This is useful for application-level tests where the original SH-4 function expects file/image/network-style binary input.

## 3. 2ndMix real application tests

The supplied corpus contains `dreamcast/2ndmix/2ndmix.elf`.

### CRC-16/CCITT

Real KallistiOS function:

`_net_crc16ccitt("123456789", 9, 0xFFFF)`

Native result:

`R0=10673` = `0x29B1`, `PC=0xFFFFFFFF`.

No native CRC replacement is used; the original SH-4 implementation is statically recompiled.

### Real 2ndMix PCX loader

`_load_pcx` from 2ndMix is compiled together with the real linked `_memcpy` implementation.

A 898-byte 1x1/8bpp PCX fixture is loaded into guest RAM using `--membin`.

Observed native result:

- prints `Image is 1x1 (1 bytes)` through the existing host `_printf` bridge;
- returns `R0=1`;
- decoded pixel at guest destination = `0x2A`;
- `_imageWidth = 1`;
- `_imageHeight = 1`;
- first palette byte = `0x11`;
- final palette byte = `0x10`.

This exercises real application struct parsing, pointer arithmetic, RLE, loops, globals, a real SH-4 `_memcpy`, and binary guest-memory input.

### Full 2ndMix `_main` CPU probe

Static recompilation:

- 120 reachable functions;
- 222 call-graph edges;
- 126 external/unresolved/native calls;
- 1,644 CFG blocks;
- 9,360 DCIR ops;
- `RAW_SH4=0`.

Calling `_main` directly skips KallistiOS video startup. The probe seeds `_vid_mode` with `vid_builtin[1]`, a valid 320x240 VGA mode from the same ELF.

Native output reaches:

- `2ndMix/KallistiOS starting`
- `Initializing new PVR system`

and then stops at guest address `0xA05F8008`, the PowerVR `PVR_RESET` register. This is the intentionally postponed hardware/MMIO boundary, not a CPU decoder failure.

## 4. Real newlib routines

Using the routines linked into 2ndMix:

- `_strlen("Dreamcast")` -> `R0=9`.
- `_strcmp("Dreamcast", "Dreamcast")` -> `R0=0`.
- overlapping `_memmove(dst=base+2, src=base, 6)` transforms `ABCDEFGH` into the expected `ABABCDEF` bytes (`0x42414241`, `0x46454443` as little-endian 32-bit probes).

## 5. Raytris regressions

Revalidated with the 0.0.22 backend:

- `_rlGetPixelDataSize(64,32,7)` -> `R0=8192`.
- `__rgba8888_to_argb4444` -> guest output `0x7135`.
- full `_main`: 428 reachable functions, 974 call-graph edges, 596 external/unresolved/native calls, 7,752 CFG blocks, 44,058 DCIR ops, `RAW_SH4=0`; it prints the GLdc welcome text and stops at `0xA05F8008` (`PVR_RESET`).

The Raytris probe now seeds the first valid built-in video mode rather than the invalid sentinel entry.

## 6. MicroPython diagnostic probe

The supplied `micropython.elf` also recompiles its `_main` with `RAW_SH4=0`:

- 160 reachable functions;
- 2,919 CFG blocks;
- 14,480 DCIR ops.

Native execution reaches the application's `(entering script)` message, then currently fails inside `_qstr_str` with an invalid guest pointer (`0xBC0EC6D8`). This is **not** classified as Dreamcast hardware/MMIO. It remains an open CPU/runtime/startup-state correctness target for later investigation.

This negative result is retained deliberately: it demonstrates why `RAW_SH4=0` alone is not enough and provides a larger real-world regression target.

## 7. Revalidated KallistiOS regressions

With the 0.0.22 backend:

- `hello.elf::_main` -> `Hello world!`, `R0=0`.
- `_memTestDataBus` -> `R0=0`.
- `_memTestAddressBus` -> `R0=0`.
- `_memTestDevice` -> `R0=0`.
- `___ieee754_sqrtf`, `FR5=9.0` -> `FR0=3.0`.

Internal CTest suite: **44/44 passed**.
