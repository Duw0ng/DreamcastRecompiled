# Validation — DreamcastRecomp 0.0.29

## Tool regression suite

```text
CTest: 47/47 passing
```

## Generated-project validation

The supplied real 2ndMix ELF was recompiled from `_main` into a standalone generated CMake project.

```text
Reachable functions: 181
RAW_SH4:             0
```

The standalone project configures and builds independently from the main DreamcastRecomp source tree.

`generated_compile_test` passes.

## Real 2ndMix integration

A five-second virtual audio capture reaches:

```text
Start
Done
Starting display
```

AICA/ARM diagnostics:

```text
ARM instructions:         225792000
FIQ entries:                  22026
ARM faults:                       0
Native slot starts:              67
Mixed stereo frames:         220500
Non-zero frames:             213134
Unsupported formats mask:       0x0
Bad AICA RAM reads:                0
```

The generated WAV is valid RIFF/WAVE PCM16 stereo at 44.1 kHz and contains sustained signal in both channels.

## What this validates

- SH-4 reaches the real 2ndMix audio code.
- The embedded romdisk S3M is actually copied to AICA RAM.
- The embedded ARM `s3mplay` firmware boots.
- Timer A generates repeated FIQs.
- The firmware completes its SH-4 handshake (`Done`).
- Native AICA slot writes occur.
- PCM8/PCM16 sample reads stay within AICA RAM during the capture window.
- Native mixer produces sustained stereo PCM.
- The graphics loop begins while the ARM/audio path continues advancing.

## What this does not validate yet

- bit-exact audio against Dreamcast hardware;
- exact AICA interpolation/filter/envelope behavior;
- cycle-accurate SH-4/ARM7/AICA timing;
- ADPCM;
- DSP effects;
- every AICA register;
- Windows WinMM playback on an actual MSVC/Windows host;
- Thumb execution for other ARM firmware;
- long-duration tracker playback/song completion.

## Host-specific note

The Linux validation build compiles and exercises the native mixer and WAV path. `--aica-play` uses a `_WIN32` WinMM branch that cannot be compiled against the Windows SDK in this Linux container. The code and linker dependency are emitted, but Windows/MSVC validation remains required.
