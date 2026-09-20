# ChuChu Rocket! acceptance — 0.0.54

## Windows evidence entering this release

The 0.0.53 acceptance run reached the title/menu and START no longer failed at `0x8C0265DA`. The next selected target was:

```text
[CALL] 0x8C0190A4 -> 0x8C026F2E
[DreamcastRecomp ERROR] No recompiled/native target registered for Dreamcast address 0x8C026F2E
```

The same run recorded `dispatch-cache=132558004/7747`, confirming the hot-call cache is active and has a >99.99% hit rate.

## 0.0.54 static result

`0x8C026F2E` is a compact indexed tail dispatcher. It loads the selector state, scales it by four, reads one target from the table at `0x8C0B8FD0`, and jumps to that target. The observed table has eight executable entries and ends at the first non-code word.

All eight entries are now covered structurally:

- `0x8C026626`
- `0x8C026EC0`
- `0x8C0266C0`
- `0x8C026AEC`
- `0x8C026B70`
- `0x8C0265DA`
- `0x8C026F2E`
- `0x8C026610`

The closure is 2,273 functions / 170,808 known instructions / 0 unknown / `RAW_SH4=0`.

## Why the old expansion was unsafe

The large handler `0x8C0266C0` loads `0x8C07FDC4` as an argument. That address is message text, not code. The old symbol-free callback fallback accepted it because the first byte pairs happened to decode as legal SH-4 for a long prefix, which recursively contaminated the closure with asset/string data and produced roughly 4,430 RAW operations. 0.0.54 rejects probable ASCII data from that fallback and also refuses dirty cross-symbol fragments.

## Next Windows acceptance

Run `run_commercial_recompiled.bat` with the same CDI, reach the title, press START, and return the first new runtime error if any. Keep several heartbeat lines before/after START. New fields `host-aica-ms` and `pvr-prof-ms` will let the next performance pass distinguish ARM7 catch-up from PVR host work.
