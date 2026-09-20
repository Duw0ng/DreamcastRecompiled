# Validation — DreamcastRecomp 0.0.40

## Main suite

```text
CTest: 47 / 47 PASS
```

## Generated runtime regression

A freshly generated `sound/sfx` standalone project builds successfully and `generated_compile_test` exits 0. Its checks include:

- P4 Store Queue -> PREF -> AICA RAM semantics.
- ARM7 Timer-A/FIQ path.
- Maple `GETCOND` active-low controller packet.
- Holly `ASIC_EVT_MAPLE_DMA` pending bit and acknowledge/clear.

## Standard KOS SFX

Real corpus `sound/sfx/sfx.elf`:

```text
reachable functions: 182
RAW_SH4:             0
native-starts:       1
mix-frames:          3572
nonzero:             2965
unsupported formats: 0
bad reads:           0
ARM faults:          0
exit:                0
```

This run uses real ROMFS WAV data and the real historical `stream.drv` ARM7 firmware. It does not use the KOS AICA command HLE.

## 2ndMix regression

Real supplied 2ndMix ELF:

```text
reachable functions: 210
RAW_SH4:             0
mix-frames:          220500
nonzero:             212557
ARM faults:          0
unsupported formats: 0
bad reads:           0
```

Five-second WAV SHA-256 remains:

```text
0506c2f483184738e5034924288f2b7b7948f0805c45ae1760a309fc4d991567
```

This is byte-identical to the stable baseline.

## KallistiOS corpus

Final 0.0.40 scan:

```text
ELF found:                    155
ELF loaded:                   155
ISA-clean all-symbol scan:    155 / 155
_main RAW_SH4=0:              155 / 155
symbol functions scanned:     202431
known SH-4 instructions:      17828452
unknown SH-4 instructions:    0
```

Final normal recompiler emission:

```text
emitted:                      155 / 155
RAW_SH4=0:                    155 / 155
```

## ChuChu Rocket! disc probe

The new `dc_disc_probe` successfully recognizes the supplied native Dreamcast CDI, parses `IP.BIN`, identifies `1ST_READ.BIN`, reports load address `0x8C010000`, and extracts both files locally for analysis. Commercial game bytes are not part of the source archive.

## Windows-only item awaiting user validation

`run_homebrew_sfx_live.bat` now isolates SFX from PVR live pacing, restores the historical KOS AICA startup defaults needed by the corpus firmware, and includes host exception diagnostics. The WinMM/SEH path must be validated on the user's Windows machine.
