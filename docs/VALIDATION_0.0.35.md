# DreamcastRecomp 0.0.35 — validation

## Main project

- Release configure/build succeeds.
- `ctest --output-on-failure`: **47/47 passing**.

## Generated project smoke

Both the single-function and whole-program emitter outputs compile standalone and pass `generated_compile_test`. The smoke now covers:

- ARM7 Timer-A/FIQ entry/return;
- real Maple DMA descriptor parsing;
- controller `GETCOND`;
- active-low raw button conversion;
- trigger/axis packet layout.

## Real 2ndMix regression

Using the supplied ELF:

- 181 reachable functions;
- `RAW_SH4=0`;
- reaches `Start -> Done -> Starting display`;
- native ARM7/AICA remains fault-free for the five-second capture;
- running with `--maple-host-input` exercises the KOS enumeration/status bridge;
- the five-second WAV SHA-256 is `0506c2f483184738e5034924288f2b7b7948f0805c45ae1760a309fc4d991567`, identical to the 0.0.34 reference interval.

## Current limitation

Maple DMA protocol responses are implemented, but completion is synchronous and the real Holly Maple-DMA completion interrupt is not yet delivered. This is the next low-level Maple acceptance target.
