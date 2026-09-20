# DreamcastRecomp 0.0.36 — KallistiOS corpus compatibility

0.0.36 uses the supplied KallistiOS demo corpus as a compatibility matrix instead of treating one demo as the development target.

## Static corpus result

The supplied archive contains **155 ELF executables**. With the final 0.0.36 analyzer/recompiler:

- 155/155 ELF load successfully.
- 155/155 generate standalone C++ projects with the normal command line; no per-demo `--max-functions` override is required.
- 154/155 `_main` graphs contain `RAW_SH4=0`.
- 154/155 are ISA-clean across all symbolized functions.
- The only remaining decoder/DCIR exception is `lua/basic/lua.elf`, function `_llex`, with three words: `0x0118`, `0x0119`, `0x011B`.
- 202,431 symbolized functions and 15,309,529 instructions were scanned; only those three words are unknown.

This means SH-4 decoding is no longer the dominant compatibility limiter for this corpus. Most remaining failures are runtime services or hardware behavior.

The retained reports are:

- `corpus/KOS_CORPUS_0.0.36.txt`
- `corpus/KOS_CORPUS_0.0.36.csv`
- `corpus/KOS_CORPUS_EMIT_0.0.36.csv`

The user-supplied ELF corpus itself is intentionally **not** redistributed in the DreamcastRecomp ZIP.

## Cross-corpus changes in 0.0.36

### Reachability / function pointers

The recompiler now closes over real `STT_FUNC` addresses found in reachable literal pools. This catches address-taken functions used through callback/vtable/newlib structures without accidentally treating linker `NOTYPE` labels as code.

The default reachable-function ceiling increased from 256 to 4096. Previously 33 larger demos required a manual `--max-functions 5000`; all 155 now emit with defaults.

### Guest allocator bootstrap

The native guest heap bridge now covers:

- `malloc/calloc/realloc/free`
- `memalign/aligned_alloc`
- `_malloc_r/_calloc_r/_realloc_r/_free_r/_memalign_r`

This removed the early newlib allocator crash observed in `pvr/plasma` and VMU-related programs.

### Embedded ROMFS stdio

The existing read-only ROMFS bridge (`fs_open/fs_read/fs_close`) now also supplies:

- `fopen`
- `fread`
- `fclose`
- `fseek`
- `ftell`

This is a read-only bootstrap implementation backed by the ELF's `_romdisk_data`; it is not a complete newlib/VFS implementation.

This change moves `pvr/bumpmap` from failing while opening its texture to a clean run/exit under the deterministic controller probe. The observed PVR run produced one logical frame and 20 TA packets.

### Timing services

`timer_spin_sleep()` and `thd_sleep()` can advance/synchronize the common device clock. An SH-4 wait no longer necessarily freezes ARM7/AICA time.

### Assertion diagnostics

KOS' default assertion callback can now be registered as a native diagnostic endpoint. Instead of failing as an unknown indirect target, the runner reports the exact expression, source line, function and optional message.

### Diagnostic printf

The bootstrap formatter accepts more integer length modifiers and floating-point conversions and no longer terminates a demo merely because an exotic conversion is encountered. It is still not claimed to reproduce every SH-4 variadic-ABI detail exactly.

## Representative runtime matrix

| Demo | 0.0.36 status | Current first blocker / result |
|---|---|---|
| `hello/hello.elf` | Runs | Clean return |
| `pvr/palette/8bpp/8bpp.elf` | Runs | Clean return |
| `pvr/pvrmark/pvrmark.elf` | Runs | ~20k TA packets, one logical frame in deterministic probe |
| `pvr/plasma/plasma.elf` | Runs | Allocator blocker removed; clean return with deterministic Start probe |
| `pvr/bumpmap/bump.elf` | Runs | ROMFS stdio blocker removed; reaches PVR and returns cleanly |
| `pvr/texture_render/texture_render.elf` | Partial | Gets through render/stat output; timing/PVR shutdown path still incomplete |
| `sound/sfx/sfx.elf` | Blocked | KOS assert: AICA command queue `valid` flag is not initialized by the standard ARM firmware |
| `rumble/rumble.elf` | Blocked | KOS assert: `fnt != NULL` in Parallax font-context creation; font/resource path needs follow-up |
| `keyboard/keytest/keytest.elf` | Waits | Standard Maple keyboard device not modeled yet |
| `libdream/mouse/mouse.elf` | Waits | Maple mouse not modeled yet |
| `vmu/vmu_beep/beep.elf` | Blocked | Reaches KOS thread runnable-list code with an uninitialized thread/scheduler structure |
| `sound/hello-ogg/vorbistest.elf` | Blocked | Null indirect callback; streaming/thread lifecycle needs modeling |

A timeout in keyboard/mouse is intentionally different from a crash: those programs are waiting for a peripheral DreamcastRecomp does not yet expose.

## Standard KOS AICA firmware finding

`sound/sfx` is now a clean acceptance target for the next ARM/AICA step. KOS `snd_init()` uploads its normal ARM driver, releases AICA and waits 10 ms before using the SH-4 -> AICA command queue. 0.0.36 now advances that wait as device time, but the firmware still never makes the queue valid. The diagnostic is:

```text
KOS assertion failed:
g2_read_32_raw(qa + offsetof(aica_queue_t, valid))
at snd_iface.c:84 in snd_sh4_to_aica:
Queue is not yet valid
```

This is no longer a host timing mystery. It points to missing/incorrect ARM7 firmware execution semantics or AICA-side memory/register behavior and should be debugged as such.

## Regression rule

2ndMix remains the general audiovisual control. After the 0.0.36 cross-corpus changes, the supplied 2ndMix ELF still reaches `Start -> Done -> Starting display`, has `RAW_SH4=0`, reports zero ARM faults/unsupported formats/bad AICA reads, and its five-second PCM SHA-256 remains:

```text
0506c2f483184738e5034924288f2b7b7948f0805c45ae1760a309fc4d991567
```
