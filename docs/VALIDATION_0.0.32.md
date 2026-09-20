# Validation — DreamcastRecomp 0.0.32

## Main regression suite

```text
47 / 47 CTest PASS
```

## Generated standalone project

The final 0.0.32 emitter regenerated the supplied real 2ndMix ELF from scratch:

```text
Reachable functions: 181
RAW_SH4: 0
generated_compile_test: PASS
```

## 15-second native AICA capture

Configuration:

```text
--probe-video-default
--probe-kos-aica-defaults
--aica-arm7
--aica-timer-rate=16/21
--aica-sh4-div=1
--aica-capture-ms=15000
```

Observed:

```text
Start
Done
Starting display

instructions=11309478
fiq=50358
timer-rate=16/21
timerA=50358
timerB=1968
timerC=1968
faults=0
idle-skipped=666066513
native-starts=403
mix-frames=661500
nonzero=653557
formats-unsupported=0x0
bad-reads=0
releases=82
```

The WAV is 44,100 Hz stereo PCM16 for exactly 15 virtual seconds.

## Tempo comparison

Against a libopenmpt render of the exact embedded `e-79014.s3m`:

```text
0.0.31 timer 1/1: best time scale ~0.765  (~1.31x fast)
0.0.32 timer 16/21: best time scale ~1.003
0.0.32 chroma-DTW slope: ~0.997
```

The calibration therefore removes the large global tempo mismatch in the reference comparison while leaving sample rate fixed at 44.1 kHz.

## Windows validation still required

Linux cannot exercise a real WinMM device. The Windows live helper now uses the calibrated timer and a much deeper queue. Record the final line after a normal run:

```text
[AICA WinMM] ... min-queue=N | max-queue=N | underrun-restarts=N
```

Target:

```text
underrun-restarts=0
min-queue > 0
```

If underruns remain with a deep queue, the next scheduler milestone must remove host/PVR burst timing from AICA production rather than increasing buffering again.
