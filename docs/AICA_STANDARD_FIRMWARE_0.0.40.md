# AICA standard KOS firmware — 0.0.40

0.0.40 promotes the standard KallistiOS `sound/sfx` path to a permanent native-AICA regression.

## Real path under test

```text
sound/sfx ELF
 -> embedded ROMFS
 -> snd_sfx_load("/rd/beep-*.wav")
 -> snd_mem_malloc / spu_memload_sq
 -> snd_sfx_play
 -> snd_sh4_to_aica
 -> shared command queue at AICA 0x010000
 -> historical KOS stream.drv on ARM7
 -> AICA slot registers
 -> native 44.1 kHz mixer
```

No `--aica-kos-hle` is required for this acceptance path.

## Store Queue correctness

The critical 0.0.39/0.0.40 fix is P4 Store Queue precedence. An SH-4 write to a P4 address such as `0xE0800000` must update the Store Queue buffer first. It must not be collapsed through the 29-bit physical alias to `0x00800000` before `PREF` commits the queue.

The generated compile regression now verifies:

1. Write `0x11223344` to `0xE0800000`.
2. AICA RAM remains unchanged.
3. Execute `PREF` for the queue.
4. AICA RAM at `0x00800000` receives `0x11223344`.

## Historical stream.drv startup state

The `stream.drv` embedded in the supplied KOS demo corpus is older than the current KallistiOS ARM source. It initializes the shared queue and channels, but relies on Timer-A / SCILV / interrupt state that KOS established before entering application `_main`.

Therefore the current generated `_main` runner still uses:

```text
--probe-kos-aica-defaults
```

This is startup scaffolding, not command/audio HLE. The real ARM7 firmware creates `queue.valid`, consumes commands and programs the AICA slots.

## Windows live runner

Use:

```bat
run_homebrew_sfx_live.bat "C:\path\to\sfx.elf"
```

Unlike the generic PVR launcher, this runner deliberately does not create a PVR window. The KOS SFX example draws its instructions directly into the video framebuffer, so coupling its live audio test to PVR frame pacing is unnecessary.

Controls:

```text
J / Space = A
K         = B
U         = X
I         = Y
Arrows    = D-pad
Q / E     = triggers / volume
Enter     = Start / exit
```

0.0.40 also installs a Windows unhandled-exception diagnostic in generated runners. A host crash should now print the Windows exception code, host address, last guest PC, ARM7 PC and generated AICA frame count.

## Deterministic validation

The final 0.0.40 deterministic SFX run reaches:

```text
RAW_SH4=0
native-starts=1
mix-frames=3572
nonzero=2965
formats-unsupported=0
bad-reads=0
ARM faults=0
```

The application returns normally after the A -> idle -> Start deterministic input sequence.
