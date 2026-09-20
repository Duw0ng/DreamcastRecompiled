# SFX low-latency / ARM7 idle-loop fix — 0.0.40.3

The first successful Windows `sound/sfx` live run on 0.0.40.2 proved that standard
KallistiOS `stream.drv`, native AICA mixing and WinMM playback were all alive. The
reported queue, however, also explained multi-second interactive latency:

- WinMM chunk: 2048 frames (~46 ms)
- prefill: 24 chunks (~1.11 s)
- 23 underrun restarts
- max submit gap: ~1.04 s

The deeper cause was not WinMM alone. The standard KOS ARM firmware idles at the
sequence around AICA RAM 0x428:

    LDR r3,[r5]
    CMP r2,r3
    BCS 0x428

The existing idle-loop recognizer only accepted the loaded register as CMP's left
operand. Here it is the right operand, so 0.0.40.2 interpreted essentially the full
45.1584 MHz ARM timeline instead of skipping the wait loop to the next timer IRQ.

0.0.40.3 accepts the loaded register on either side of a simple register CMP. The
existing SH-4 AICA-RAM write epoch and IRQ boundary rules remain in force, so the
fast-forward is still invalidated when the external state that the loop observes can
change.

Linux acceptance result for one second of host-clock SFX idle audio:

| Metric | 0.0.40.2 | 0.0.40.3 |
|---|---:|---:|
| wall time | ~7.14 s | ~1.06 s |
| ARM instructions | 45,125,632 | 2,212,071 |
| idle-skipped ARM steps | 0 | 42,912,864 |
| mixed frames | 44,100 | 44,100 |
| ARM faults | 0 | 0 |

This is roughly a 6.7x wall-time speedup in the local acceptance environment and,
more importantly, proves that the intended idle fast-forward is active.

The WinMM reservoir is also reduced for interactive SFX:

- 1024-frame chunks (~23.2 ms)
- 4-chunk prefill (~92.9 ms)
- 6-chunk maximum queue (~139.3 ms)

The goal is to keep button-to-sound latency in the ~0.1 s class instead of allowing
seconds of queued silence. Windows remains the authoritative acoustic test.
