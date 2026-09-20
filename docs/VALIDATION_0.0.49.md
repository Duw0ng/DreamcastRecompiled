# DreamcastRecomp 0.0.49 — validation

## Core regression

- CTest: **47/47 PASS** after GD-ROM PAUSE, ARM7 user-bank block-transfer and TA parser changes.
- Commercial raw closure remains **2,194 functions / 165,555 known SH-4 / 0 unknown / RAW_SH4=0**. No new ChuChu-specific function-address seed was added.
- A clean post-version-bump commercial generation compiled both `generated_compile_test` and the native `dreamcast_program`; `generated_compile_test` returned **RC=0**.

## Windows 0.0.48 evidence that defined this release

The supplied Windows run reached a recognizable retail `PRESENTED BY SEGA` splash and sustained PVR activity:

- 5,139 TA packets;
- 142 logical frames/renders/page flips;
- 31,760 Store Queue writes;
- 4,424 TA PREF commits;
- stop at BIOS GD-ROM command `0x16`;
- commercial ARM7 stopped at `LDM/STM ^ user-bank transfer is not implemented`.

The visible magenta background and transient dark sectors provided the first useful visual PVR correctness signal.

## 0.0.49 runtime checks

`GDCC_PAUSE` (`0x16`) is now accepted as a no-payload drive-state command. The previous unimplemented-command branch is no longer taken for that opcode.

ARM7 `LDM/STM ^` now supports user-bank transfers and SPSR-to-CPSR restoration on PC loads. In the local commercial diagnostic run ARM7 progressed from the previous ~120k-instruction boundary into tens of millions of instructions without returning to the `LDM/STM ^` unsupported path. The earlier blank-reset-vector fallthrough may still be recorded as the first historical ARM7 fault before Katana uploads/re-releases its firmware; that remains separately observable.

The TA software frontend now follows the PVR2 format matrix rather than treating every polygon vertex as a 32-byte packed-ARGB vertex. It assembles 64-byte headers/vertices, decodes four-float and intensity colors, preserves 16/32-bit UV variants and keeps two-volume packets aligned.

A diagnostic dump of the active registration framebuffer at the SEGA splash contained no pure magenta (`#FF00FF`) pixels. The dominant background was `(254,254,254)`, with the logo rendered in blue and `PRESENTED BY` in gray. This is a diagnostic of color/stream decoding, not yet a claim that every later texture/blend/tile mode is correct.

An intentional 80-TA-packet probe reached:

```text
TA packets=80
vertices=81
triangles=24
sprites=15
tex-samples=115136
logical-frames=2
render-starts=2
render-done=2
page-flips=1
```

`ISP_START` is still not observed on that early path; the existing list/scene heuristic remains explicit.

## Full KallistiOS corpus regression

The final 0.0.49 corpus scanner was run once against the existing 155-ELF cache; the source archive was not re-extracted.

```text
ELF found:                    155
ELF loaded:                   155
ISA-clean:                    155
Functions scanned:            202431
Known SH-4 instructions:      17953662
Unknown SH-4 instructions:    0
_main call graphs scanned:    155
_main RAW_SH4=0:              155
```

## Real KallistiOS sound/sfx runtime regression

A fresh generated runner from the cached real `sound/sfx/sfx.elf` compiled and completed successfully after the ARM7 block-transfer changes. Final relevant counters were:

```text
runner RC=0
reachable functions=182
RAW_SH4=0
ARM7 faults=0
native-starts=1
mix-frames=3302
nonzero PCM frames=2696
unsupported AICA formats=0
bad AICA reads=0
idle-skipped=3161527
```

This verifies that the new ARM7 `LDM/STM ^` semantics do not regress the established KallistiOS standard-firmware SFX path.

## Packaging rule

The release source contains no CDI, `IP.BIN`, `BOOTSTRAP.BIN`, generated commercial C++, or extracted retail bytes. Those are generated locally from the user's disc image.
