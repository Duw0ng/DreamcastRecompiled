# DreamcastRecomp v0.1.1 — Crazy Taxi 2 validation

## Regression addressed

The v0.1 universal path could terminate before boot completion with a missing target at `0x8C16BFB6`. After that target was recovered, the consolidated path could still lose historical CT2 commercial-boot state, including the Shinobi allocator postcondition and the scrambled 32-byte executable staging expected by the Katana bootstrap.

## Clean analyzer result

The final v0.1.1 candidate was regenerated from the test CDI using only the public source tree and `profiles/crazy_taxi_2_known_seeds.txt` — no pre-generated commercial game code was used.

- registered/reachable functions: **4,043**
- reachable/known SH-4 instructions: **351,385**
- unknown SH-4: **0**
- `RAW_SH4`: **0**
- `0x8C16BFB6`: recovered and emitted
- historical supplemental CT2 roots: included in the public seed profile
- `generated_compile_test`: **RC=0**

## Runtime result

The freshly generated runner progressed through commercial bootstrap, VMU flow, title/menu loading and into real 3D gameplay. A framebuffer taken at the final controlled checkpoint showed the playable taxi scene with HUD, road traffic and city geometry.

The run stopped only because the diagnostic packet limit was intentionally set to **10,000,000 PVR packets**. The last complete heartbeat before that stop recorded:

- GD-ROM requests: **300**
- PVR packets: **9,959,626**
- PVR frames: **2,670**
- PVR renders: **2,670**
- PVR flips: **2,660**
- missing-target errors: **0**
- SH-4 faults: **0**
- final termination: intentional `PVR probe packet limit reached (10000000)`

## Project regression suite

The v0.1.1 source tree passed the full project regression suite after the CT2 emitter/staging changes: **54/54 tests PASS**, including generated-program compilation and controller visual-tester packaging.

## Distribution note

No Crazy Taxi 2 CDI, extracted executable, screenshot, or pre-generated commercial game code is included in the public release. The compatibility profile stores analysis/runtime metadata and regenerates native code from the user's own disc image.
