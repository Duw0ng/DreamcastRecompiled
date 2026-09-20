# DreamcastRecomp roadmap — after 0.0.49

## 0.0.49 checkpoint

- First recognizable retail SEGA splash confirmed by the Windows test.
- Valid TA float/intensity color formats and 64-byte parameter alignment implemented.
- Commercial PVR window switched to completed-frame presentation by default.
- GD-ROM PAUSE (`0x16`) implemented.
- ARM7 `LDM/STM ^` user-bank forms implemented.
- Commercial closure remains zero-RAW/zero-unknown without title-address patches.

## 0.0.50 — post-splash / first menu scene

Primary target: follow the real Windows 0.0.49 run beyond the old `0x16` stop and classify the next dependency.

1. Validate completed-frame output after the SEGA splash; fix reusable PVR texture/blend/tile semantics only where the trace proves a mismatch.
2. Replace the remaining heuristic render start when the actual Katana scene-completion/ISP-start mechanism is identified.
3. Continue GD-ROM/CDDA state commands encountered after PAUSE rather than pre-implementing unrelated BIOS calls.
4. Validate Maple input once the title/menu begins polling it.
5. Keep commercial AICA running and identify the first remaining ARM7/AICA semantic or timing fault from the real firmware.

Acceptance target: a stable post-splash/menu frame with no debug fallback colors and a new concrete hardware/runtime boundary later than 0.0.49.

No title-specific address seeds, protected RAM regions, or fake framebuffer patches should be introduced.
