# KallistiOS corpus status — DreamcastRecomp 0.0.27

Supplied corpus: **155 ELF files / 202,431 `STT_FUNC` symbols**.

Current result:

- `_main` reachable graphs with `RAW_SH4=0`: **155 / 155**.
- broad all-function ISA-clean ELF files: **154 / 155**.
- broad all-function unknown words: **3**.
- the remaining three words (`0x0118`, `0x0119`, `0x011B`) occur in Lua `_llex` data-like bytes and are not RAW instructions reachable from any scanned `_main` graph.

CPU cleanliness does not imply complete program compatibility: PVR/AICA/Maple/GD-ROM, startup state, interrupts and other Dreamcast hardware still determine how far a program can run.

The latest retained full corpus report is `KOS_CORPUS_BASELINE_0.0.26.txt` / `.csv`; 0.0.27 did not change SH-4 decoding/IR coverage.
