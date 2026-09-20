# Validation — DreamcastRecomp 0.0.31

## Core regression suite

- CTest: 47/47 passing.
- Emitter regression confirms buffered WinMM prefill/underrun logic and native release state are emitted.
- Windows WinMM block separately passed a C++20 syntax-only compile using local API stubs.

## Supplied real 2ndMix ELF

Generation:

- reachable functions: 181
- RAW_SH4: 0
- embedded ROMFS and real `/rd/e-79014.s3m` path retained

Five-second native AICA run:

```text
Start
Done
Starting display
[AICA ARM7] instructions=6836635
fiq=22026
faults=0
idle-skipped=218955362
native-starts=171
mix-frames=220500
formats-unsupported=0x0
bad-reads=0
releases=29
```

The generated WAV is stereo PCM16 / 44.1 kHz and deterministic.

SHA-256:

```text
a470a85b28ccc464f7fe88a4b784f1caae6a610a12fada569df9eaf3fe255398
```

`--aica-sh4-div=1` and `--aica-sh4-div=4` produced the same SHA-256 for this five-second capture. Divisor 1 generated five virtual seconds in approximately 3.54 seconds on the Linux validation host.

## Remaining validation gap

The actual WinMM device path cannot be acoustically validated on this Linux host. The next Windows run should report its `underrun-restarts` counter and subjective presence/absence of microcuts.
