# DreamcastRecomp roadmap after 0.0.56

1. Re-run ChuChu Rocket! and confirm the memory-card screen no longer faults at `0xA012BAC0`.
2. Inspect `maple-last` / `maple-nonctl` when confirming slot A1.
3. If the game issues Maple traffic to unit 1, implement a generic VMU device (DEVINFO, GETMINFO, block read/write/sync) with persistent host storage.
4. Continue guest compatibility until a new game-code or hardware boundary appears.
5. Resume PVR performance work only after the save/VMU path is usable.
