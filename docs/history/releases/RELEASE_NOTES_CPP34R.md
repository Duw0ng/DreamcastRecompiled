# cpp34r — Network Fast Path + Submit Trace

Base: cpp34q / cpp34p online stack, preserving ABI1 and the Late-AOT overlay target `0x8C036102`.

## Performance
- Hot modem MMIO no longer takes the network-state mutex after first access (thread-local runtime/state cache).
- The runtime plugin host also caches the network descriptor per thread with generation invalidation, removing its global mutex from every modem read/write/tick.
- Normal register reads no longer copy the 4 KiB DSP image + FIFO into `DCRuntime` on every poll.
- Full diagnostic mirrors are throttled to 50 ms guest-time; scalar connection state remains current.
- UDP/TCP host socket polling is throttled to 1 ms guest-time instead of every plugin tick.
- Connected ticks have an early return when no serial byte, host poll, diagnostic sync, or submit sample is due.
- Serial pacing catches up in bounded batches (max 16 bytes) instead of one byte per host tick.

## Submit trace
When cooked Dreamcast START (`1<<3`) has a rising edge while connected, a 3-second trace is armed. It logs:
- guest `current_pc`
- Maple button state/change count
- modem/PPP/IP/TCP counters
- Flash program and VMU write counters
- first 16 TBUFFER bytes after START, if any

This distinguishes an input/handler problem from a save/config path or a real network submit.

## ABI/cache
`runtime/include/dc_runtime.hpp` and the guest emitter remain unchanged from cpp34q. The existing 5185-function guest is reused.

## Measured microbenchmarks
See `VALIDATION_CPP34R.md`. The end-to-end host path for 2,000,000 modem reads improved from ~194.6 ms to ~38.2 ms in the validation container, while the no-work connected tick path improved from ~230.8 ms to ~11.0 ms.
