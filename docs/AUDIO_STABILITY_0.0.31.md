# 2ndMix audio stability — DreamcastRecomp 0.0.31

## User-observed symptom

0.0.30 removed the large sample corruption/noise, but live Windows playback could still micro-stutter. The corrected WAV was much cleaner, though small hard transitions remained possible in the simplified slot model.

## Mixer-side change

`KYONEX` remains a global strobe across all 64 slots. When a strobe finds an active slot with `KYONB=0`, 0.0.31 no longer drops the slot instantly. It enters a release phase and increases AEG-style attenuation until the voice becomes silent.

The release slope is derived from the slot's RR, KRS and pitch state. This is intentionally a partial AEG implementation: attack/decay tables are still future fidelity work. The release is applied before the final stereo mix, through the same attenuation path as TL/DISDL/DIPAN, so this is not a post-WAV smoothing filter.

The global `0x2800` master-volume/mono control is also honored.

## Windows host-output change

0.0.30 could start `waveOut` after roughly one 46 ms buffer. Average emulation speed could be above real time while short PVR or scheduler bursts still emptied that shallow queue.

0.0.31 uses:

- 1024-frame stereo PCM16 chunks (~23.2 ms);
- 12 chunks of startup prefill (~279 ms);
- a bounded maximum of 32 queued chunks (~743 ms);
- pause/refill/restart if the queue is ever observed fully drained;
- a final `underrun-restarts` counter.

The queue is deliberately deep for the current research runner. Latency can be reduced after the common scheduler and ARM execution path are fast enough to guarantee stable production.

## What to report from Windows

Run `run_homebrew_2ndmix_live.bat` and, after closing the PVR window, note the line:

```text
[AICA WinMM] chunks=... | prefill-ms=278 | underrun-restarts=...
```

The immediate target is `underrun-restarts=0`. If audio still microcuts with zero restarts, the problem is in guest/mixer timing rather than the WinMM queue.
