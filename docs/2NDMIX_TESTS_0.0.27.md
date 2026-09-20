# 2ndMix tests — DreamcastRecomp 0.0.27

The scripts are intentionally split so a failure identifies the layer that regressed.

| Script | Purpose | Interactive |
|---|---|---|
| `run_homebrew_2ndmix_crc.bat` | integer/bit/loop correctness using real KOS CRC | no |
| `run_homebrew_2ndmix_pcx.bat` | PCX parsing, RLE, structs, pointers, RAM, real memcpy | no |
| `run_homebrew_2ndmix_graphics_probe.bat` | deterministic full `_main` + PVR TA traffic, fixed packet stop | no |
| `run_homebrew_2ndmix_perf.bat` | headless PVR profile with no window/frame-file overhead | no |
| `run_homebrew_2ndmix_live.bat` | live Win32 PVR preview with frame clearing and 60 Hz deadline pacing | yes |
| `run_homebrew_2ndmix_main_probe.bat` | compatibility alias for the graphics probe | no |
| `run_homebrew_2ndmix_suite.bat` | CRC + PCX + deterministic graphics regression | no |

The live test is not part of the automatic suite because it requires user interaction. `--pvr-frame-sync` rotates the logical front/back buffers instead of accumulating geometry from previous scenes.

2ndMix tracker music is still skipped in the graphics scripts because it uploads its own ARM7 `s3mplay` firmware. Standard KOS AICA command-queue HLE cannot replace that firmware transparently.
