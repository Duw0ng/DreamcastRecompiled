# Validation — DreamcastRecomp 0.0.40

## Main suite

- CTest: **47/47 PASS**.
- Generated standalone compile/runtime regression: PASS.
- Store Queue/PREF generated regression: PASS.

## KallistiOS corpus

Final scanner and normal recompiler pass over the user-supplied 155 ELF corpus:

- 155/155 ELF loaded
- 155/155 ISA-clean across symbolized functions
- 155/155 `_main` call graphs with `RAW_SH4=0`
- 155/155 emitted by `dc_recomp --max-functions 4096`
- 202,431 symbolized functions scanned
- 17,828,452 known SH-4 instructions
- 0 unknown instructions

Reports: `corpus/KOS_CORPUS_0.0.40.txt`, `.csv`, and `KOS_CORPUS_EMIT_0.0.40.csv`.

## Standard KOS SFX path

Real `sound/sfx` ELF:

- reachable functions: 182
- RAW_SH4: 0
- standard `stream.drv` boots and initializes its queues
- native AICA starts: 1
- unsupported formats: 0
- invalid sample reads: 0
- deterministic proof WAV: 795 stereo frames at 44.1 kHz
- non-zero frames: 196
- peak: 12031 L/R
- WAV SHA-256: `0f5ec39f4ee175cf7303bcc01a5078e7c6a0d2b13dfd4b1d53437725d3b38540`

This short deterministic probe is only a correctness witness. Windows live testing is intended to validate complete beep playback and channel/volume interaction.

## 2ndMix regression

Real supplied 2ndMix ELF:

- reachable functions: 210
- RAW_SH4: 0
- reaches `Start -> Done -> Starting display`
- 220,500 stereo frames / 5.000 s / 44.1 kHz
- ARM faults: 0
- unsupported AICA formats: 0
- invalid sample reads: 0
- PCM SHA-256: `0506c2f483184738e5034924288f2b7b7948f0805c45ae1760a309fc4d991567`

The file is byte-identical to the established 0.0.34+ reference.

## SEGA Swirl reconnaissance

The supplied CDI was inspected separately and is not included in the package. The game payload is a Windows CE SH-4 PE executable; see `SEGA_SWIRL_STATIC_0.0.40.md`.
