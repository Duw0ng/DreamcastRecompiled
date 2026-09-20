# DreamcastRecomp 0.0.22 — real homebrew validation

0.0.22 shifts emphasis from decoder coverage alone to semantic/native validation using the supplied `raylib/raytris/raytris.elf` homebrew binary.

The ELF itself is **not** redistributed by DreamcastRecomp.

## 1. Critical executable-section data fix

Real SH-4 binaries may place literal pools, jump tables and floating-point constants inside executable `.text`. Earlier generated images copied allocated data sections but skipped executable sections because code itself was statically recompiled. That is insufficient: guest code can still read bytes from `.text` as data.

0.0.22 now maps every `SHF_ALLOC` section that belongs to Dreamcast main RAM into the runtime image, including `.text`.

This bug was exposed by Raytris `_rlGetPixelDataSize`, whose FPU constants are read from `.text`.

## 2. Raytris `_rlGetPixelDataSize`

Native invocation:

```text
R4 = 64   (width)
R5 = 32   (height)
R6 = 7    (pixel format)
```

Result:

```text
Reachable functions: 1
RAW_SH4: 0
R0=8192
FR0=1.5
PC=0xFFFFFFFF
```

Expected size is 64 * 32 * 4 = 8192 bytes.

Run on Windows:

```bat
run_homebrew_raytris_pixeldata.bat
```

## 3. Raytris RGBA8888 -> ARGB4444 conversion

The real homebrew function `__rgba8888_to_argb4444` is executed with source bytes:

```text
RGBA = 12 34 56 78
```

The generated runner's new post-execution memory peek reports:

```text
[PEEK16] 0x8C100100 = 0x7135
```

That is the expected packed ARGB4444 value (A=7, R=1, G=3, B=5).

Run:

```bat
run_homebrew_raytris_color.bat
```

## 4. Real hand-written `_memset` assembly from the linked KOS/newlib image

`_memset` is exported as a GLOBAL `STT_NOTYPE` assembly entry, not `STT_FUNC`. 0.0.22 teaches call/function analysis to treat called GLOBAL/WEAK NOTYPE symbols in executable sections as callable code while keeping the corpus-wide "all functions" metric restricted to `STT_FUNC` so data labels are not misclassified as functions.

Native test:

```text
R4 = 0x8C100000
R5 = 0xAB
R6 = 16
```

Result:

```text
[PEEK32] 0x8C100000 = 0xABABABAB
[PEEK32] 0x8C10000C = 0xABABABAB
PC=0xFFFFFFFF
```

## 5. Full Raytris `_main` CPU probe

The full `_main` static graph now recompiles as:

```text
Reachable functions: 428
Call-graph edges: 974
External/native calls: 596
CFG blocks: 7788
DCIR ops: 44171
RAW_SH4: 0
```

Calling `_main` directly skips the normal KallistiOS startup, so the probe injects the supplied ELF's `_vid_mode` pointer to its built-in video-mode table. With that minimum startup state, native execution reaches:

```text
Welcome to GLdc! Git revision: 1.1-656-g105e
```

and then stops at:

```text
Dreamcast memory access is outside main RAM:
address=0xA05F8008 ... guest_pc=0x8C071328
```

`0xA05F8008` is the PowerVR reset register location used by KallistiOS (`PVR_RESET`, offset `0x0008` from the PVR register base `0xA05F8000`). This is an intentional boundary for the current CPU-first phase: Dreamcast hardware/MMIO is not emulated yet.

Run:

```bat
run_homebrew_raytris_main_probe.bat
```

This is **not** a claim that Raytris runs as a playable Windows game yet. It demonstrates that a large real homebrew CPU graph can be statically translated, compiled to the host, enter GLdc initialization, and reach the first deliberately deferred PVR hardware access without encountering unknown reachable SH-4 instructions.

## 6. Better runtime diagnostics

Generated runtimes now include the failing guest address, physical alias, access width and current guest basic-block PC in out-of-main-RAM errors. Generated runners also support:

```text
--peek8=ADDRESS
--peek16=ADDRESS
--peek32=ADDRESS
```

for post-execution validation of guest memory.
