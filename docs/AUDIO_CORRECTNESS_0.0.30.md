# 2ndMix audio correctness — DreamcastRecomp 0.0.30

## Symptom from 0.0.29

The first native 2ndMix capture was audible but had three obvious defects reported during real listening:

- strong noisy / saturated character;
- heavy chopping in live WinMM playback;
- the WAV itself also sounded wrong, proving that not all defects were a Windows backend problem.

The old five-second capture had a large negative PCM mean (about -2010/-1930 L/R). It did not numerically clip at int16 limits, but the large DC displacement and wrong sample contents made it sound heavily distorted.

## Root cause 1 — ARM7TDMI unaligned LDR semantics

The embedded `s3mplay` firmware walks packed 16-bit S3M instrument parapointers using ARM word loads at addresses with offsets 0, 2, 4, 6, ... . On ARMv4/ARM7TDMI, an unaligned word `LDR` reads the aligned word and rotates it right by `8 * address[1:0]`.

0.0.29 aligned the address down but did not rotate the value.

That caused adjacent instrument parapointers to be duplicated. In the supplied module the internal sample allocation sequence became corrupted and eventually advanced beyond the end of the real S3M data. Several AICA slots therefore pointed at RAM regions containing long runs of `0x80` rather than the intended waveform data.

0.0.30 implements the rotate in both:

- `src/aica/arm7.cpp`;
- the standalone ARM7 source embedded by `src/codegen/cpp_emitter.cpp`.

A dedicated unit regression executes `LDR r2,[r1,#2]` against the word `0x11223344` and requires the ARM7TDMI result `0x33441122`.

## Root cause 2 — incomplete AICA slot semantics

The initial mixer treated key control and gain as simplified per-slot state. 0.0.30 tightens the path used by 2ndMix:

- `KYONEX` is a global execute strobe over all 64 `KYONB` states;
- PCM8 is treated as signed 8-bit sample data and expanded to the 16-bit mixer domain;
- PCM16 remains signed little-endian;
- PCM16 sample addresses are aligned appropriately;
- sample playback uses loop-aware linear interpolation;
- `TL`, `DISDL` and `DIPAN` are converted through logarithmic-style AICA attenuation rather than simple linear multipliers;
- already-running `KYONB` slots retain position during global key execution.

This is still a subset of AICA. AEG/FEG, LFO, ADPCM and DSP remain future work unless a title requires them sooner.

## Root cause 3 — live host underruns

Even after the sample corruption is fixed, 0.0.29 could not feed WinMM in real time on the validation host. Five virtual seconds required roughly 22 seconds of host time because the ARM interpreter repeatedly executed the same `LDR/CMP/BLE` polling loop while waiting for Timer A.

0.0.30 recognizes a small side-effect-free ARM polling-loop form and fast-forwards its virtual ARM clock to the next enabled AICA timer interrupt. The optimization persists across SH-4 scheduling calls but is invalidated whenever SH-4 writes AICA RAM, so a guest producer cannot silently change the polled value behind the optimization.

For the five-second 2ndMix integration test:

- interpreted ARM instructions drop from about 225.8 million to about 6.84 million;
- about 219.0 million idle ARM scheduling steps are skipped;
- Timer A still produces 22,026 FIQ entries;
- the mixer still produces exactly 220,500 frames.

The generated WAV is identical for `--aica-sh4-div=1`, `2`, `3` and `4` in the tested segment. The current instruction-ratio scheduler is still approximate, so the Windows live helper uses divisor 1 as a temporary throughput mode while the offline/reference path can retain divisor 4.

## Objective five-second measurements

### 0.0.29 capture

- mean L/R: approximately `-2010 / -1930`;
- RMS L/R: approximately `4093 / 3798`;
- absolute peaks L/R: approximately `22110 / 21733`.

### 0.0.30 capture

- mean L/R: approximately `-73 / -99`;
- RMS L/R: approximately `1867 / 1369`;
- absolute peaks L/R: approximately `13829 / 14316`;
- int16 clipping samples: `0`;
- non-zero stereo frames: about `96.89%`.

The dramatic DC error is gone. Gain/RMS is not expected to numerically match a software tracker renderer because that renderer does not reproduce the AICA signal chain exactly; reference listening is still required.

## Real supplied-ELF result

The clean regenerated project still reports:

```text
DreamcastRecomp 0.0.30 native runner
Reachable functions: 181
...
Start
Done
Starting display
[AICA ARM7] instructions=6836635 ... fiq=22026 ... faults=0 ...
             idle-skipped=218955363 ... native-starts=171
             mix-frames=220500 ... formats-unsupported=0x0 ... bad-reads=0
```

On the Linux validation host, the five virtual seconds are generated in about 3.5 seconds using `--aica-sh4-div=1`, which is fast enough for a bounded real-time output queue in principle. The actual WinMM branch still has to be listened to on Windows.
