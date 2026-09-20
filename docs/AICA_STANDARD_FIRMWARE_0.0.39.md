# Standard KallistiOS AICA firmware — 0.0.40

## Acceptance target

`examples/dreamcast/sound/sfx` is the independent standard-AICA regression. Unlike 2ndMix, it uses KallistiOS `stream.drv` and the SH-4/AICA shared command queues.

## Root cause fixed

KOS uploads the ARM firmware with Store Queues. A destination such as `0xE0800000` is a P4 Store Queue address whose QACR mapping ultimately targets sound RAM. The old generated runtime applied the SH-4 29-bit physical alias first, saw `0x00800000`, and wrote directly into AICA RAM. The real SQ buffer remained zero. `PREF` then committed those zeros over the firmware.

0.0.40 decodes Store Queue addresses before P1/P2/P3 aliasing. The generated regression proves that a write to `0xE0800000` remains invisible in AICA RAM until `dc_pref()` commits the SQ.

## Result

With the KOS startup defaults explicitly seeded (the runner still jumps directly to `_main`), `stream.drv` now:

1. boots from AICA RAM;
2. initializes command/response queues;
3. services a real SH-4 channel command;
4. programs a native AICA slot;
5. reaches key-on;
6. produces non-zero stereo PCM.

The test uses the real embedded ROMFS WAV path when the demo includes its assets. It does not synthesize `queue.valid`.

## Remaining work

- Validate the full interactive A/B/X/Y/D-pad/trigger SFX demo on Windows.
- Remove `--probe-kos-aica-defaults` by reproducing the missing pre-main KOS/hardware initialization.
- Add ADPCM formats 2/3, complete AEG and exact pan/volume behavior.
- Continue ARM7 performance/correctness validation with more firmware.
