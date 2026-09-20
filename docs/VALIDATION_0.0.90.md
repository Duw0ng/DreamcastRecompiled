# DreamcastRecomp 0.0.90 validation

0.0.90 is an audio-focused update built from the stable 0.0.87/0.0.89 line. The failed 0.0.88 CPU-inlining experiment is not included.

## Root cause from 0.0.89 ChuChu run

The live heartbeat showed real AICA activity (`aica-nz > 0`, 16 slots active) but `aica-fmt=0x4`, meaning slot format 2 was observed and not implemented. The WinMM summary also showed severe starvation: 26,996 synthetic silence chunks out of 28,541, producer gaps above 2.5 seconds, and over 11k starvation events.

## 0.0.90 changes

- Supports PCMS 0/1/2/3: PCM16, PCM8, Yamaha AICA ADPCM, AICA ADPCM long stream.
- ADPCM decoder uses predictor + quantizer state with the AICA quantizer tables and low-nibble-first packing.
- Normal ADPCM snapshots predictor/quantizer at loop start and restores them on loop. Long-stream mode preserves decoder state while only the address wraps.
- Native 44.1 kHz mixing can be paced by host wall time, independent from interpreted ARM7 throughput.
- Timer/ARM execution no longer duplicates PCM generation while host-paced live mixing is active.
- Host ARM7 maintenance is limited to 256 steps per sync.
- WinMM reservoir: 512-frame chunks, 32 ring chunks, 6-chunk prefill, 8-chunk target, 4-chunk low-water.

## Validation performed

- Full source build succeeds.
- CTest: 47/47 pass.
- Commercial ChuChu closure: 2965 functions, 295215 known SH-4 instructions, 0 unknown, RAW_SH4=0, branch-selected calls=1.
- Generated commercial `dc_runtime.cpp` and `generated_runner.cpp` compile as C++20 on Linux.
- A deterministic 20-nibble ADPCM vector matches the Flycast AICA decoder equation/quantizer progression.
- Synthetic loop test verifies format 2 restores loop-start state and format 3 preserves stream state.
- 10 ms host-clock mixer test produces exactly 441 native frames.

## Expected Windows heartbeat

During ChuChu live playback:

- `aica-fmt=0x0`
- `audio-clock=host/...`
- `audio-ring` should spend meaningful time above zero instead of maxing at 1-2 chunks
- `audio-starves` and `silence-chunks` should fall drastically
- `producer-gap-ms` / `winmm-gap-ms` should remain far below the multi-second gaps seen in 0.0.89

