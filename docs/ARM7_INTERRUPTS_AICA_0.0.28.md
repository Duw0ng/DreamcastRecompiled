# DreamcastRecomp 0.0.28 — ARM7/AICA interrupts and timers

## Goal

0.0.27 proved that generated DreamcastRecomp projects can execute native ARM-state code from AICA RAM. 0.0.28 extends that path far enough for firmware to receive Timer A through the AICA interrupt controller and return from a real ARM7 FIQ handler.

This milestone deliberately keeps the existing KOS audio HLE as a separate reference path. Native `--aica-arm7` does not call the HLE queue.

## ARM7 exception model

The ARM7 interpreter now exposes IRQ/FIQ input lines and samples them before fetching the next ARM instruction. FIQ has priority over IRQ.

On FIQ entry:

1. save the old CPSR to `SPSR_fiq`;
2. bank the old register set and load the FIQ bank;
3. enter FIQ mode;
4. set I and F masks and clear T;
5. set `LR_fiq` for the standard ARM exception-return sequence;
6. jump to vector `0x0000001c`.

IRQ uses the corresponding IRQ bank/SPSR and vector `0x00000018`.

The existing data-processing implementation already supports the `S` + `Rd=PC` SPSR restore semantics required by the common firmware return:

```asm
subs pc, lr, #4
```

Dedicated tests cover FIQ entry/return and IRQ masked/unmasked behavior.

## AICA interrupt registers

The generated runtime now has explicit state for:

| AICA offset | Meaning |
|---|---|
| `0x2890` | Timer A |
| `0x2894` | Timer B |
| `0x2898` | Timer C |
| `0x289c` | SCIEB |
| `0x28a0` | SCIPD |
| `0x28a4` | SCIRE |
| `0x28a8` | SCILV0 |
| `0x28ac` | SCILV1 |
| `0x28b0` | SCILV2 |
| `0x28b4` | MCIEB |
| `0x28b8` | MCIPD |
| `0x28bc` | MCIRE |
| `0x2d00` | ARM interrupt code |
| `0x2d04` | ARM interrupt acknowledge window |

For an enabled pending sound interrupt, the runtime chooses the lowest active source and builds the 3-bit ARM interrupt code from the three SCILV bit planes.

KallistiOS' standard AICA ARM firmware uses Timer A pending source bit `0x40` and SCILV values `0x18`, `0x50`, `0x08`, which map source 6 to interrupt code `2`. The 0.0.28 generated-project regression exercises that exact routing.

Timer B/C are currently represented as adjacent pending sources 7/8. That part is intentionally marked provisional until a real firmware trace exercises them.

## Timer scheduling

Timer state contains an 8-bit counter, a 3-bit prescale and a deterministic accumulator. The current model advances timers according to fetched ARM instructions.

`--aica-timer-div=N` controls the number of fetched ARM instructions per base timer increment before applying the prescale. The default is a deterministic bootstrap value, not a claim of Dreamcast cycle accuracy.

Timer advancement occurs after individual ARM instruction fetches inside a scheduling slice. Therefore an overflow can assert FIQ while firmware is still inside one long ARM execution slice.

## Reset and scheduling controls

Native AICA ARM execution remains opt-in:

```text
--aica-arm7
--aica-arm7-slice=N
--aica-arm7-boot=N
--aica-timer-div=N
```

- `--aica-arm7-slice=N`: normal deterministic execution budget when SH-4/AICA shared state is touched.
- `--aica-arm7-boot=N`: minimum execution budget immediately after SH-4 clears AICA ARM reset.
- `--aica-timer-div=N`: temporary deterministic timer pacing control.

Clearing SH-4 `SNDREG 0x2c00` bit 0 resets the native ARM context to PC 0, releases the reset line and executes the configured boot slice.

## Generated-project end-to-end regression

The generated smoke firmware performs this path:

```text
ARM reset vector
  -> branch to 0x20
  -> MRS CPSR
  -> clear F bit
  -> MSR CPSR_c
  -> spin

AICA Timer A overflow
  -> SCIPD bit 6
  -> SCIEB bit 6 enabled
  -> SCILV route = code 2
  -> ARM FIQ line
  -> vector 0x1c
  -> handler increments R0
  -> reads 0x00802d00 into R5 (must equal 2)
  -> writes 0x40 to SCIRE
  -> SUBS pc,lr,#4
```

The test requires at least one FIQ exception, at least one Timer A overflow, a handler increment and observed interrupt code `2`.

## Known limitations after 0.0.28

- Thumb state is still detected and reported as unsupported.
- Timer pacing is deterministic/instruction-based, not device-clock accurate.
- AICA channel/slot registers are not yet complete enough for native sustained playback.
- There is no continuous native multi-channel mixer yet.
- ADPCM, pitch, envelope and full loop behavior remain pending.
- SH-4/ARM7 execution is still opportunistically scheduled around AICA interaction plus boot slices rather than a unified device-time scheduler.
- The source package does not contain the actual 2ndMix ELF/ARM tracker blob, so `s3mplay` cannot yet be validated end-to-end from this package alone.
