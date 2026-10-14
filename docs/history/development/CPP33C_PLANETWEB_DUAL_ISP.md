# DreamcastRecompiled v1.2 cpp33c — PLANETWEB_DUAL_ISP

Built from cpp33b after ChuChu Rocket reached the browser/network error screen but telemetry stayed at `m0/c0/rw0,0/ppp0,0`. The log showed hundreds of thousands of FlashROM lookups and no modem MMIO, indicating an ISP-profile compatibility gate before the modem.

Changes:
- Preserve the v1.2/cpp33b SH-4, PVR, AICA, GD-ROM and controller behavior.
- Seed North-American PlanetWeb browser records 0x80/0x82/0x83/0x84 and ISP blocks C0/C1/C2.
- Keep DreamKey C6/C7/C8 in parallel for cross-region compatibility.
- PlanetWeb/DreamKey credentials: `dream` / `dreamcast`.
- Standard dial number: `111-1111`.
- DNS1: `46.101.91.123`; DNS2: `1.1.1.1`.
- Correct Katana MSB-first allocation bitmap for logical indexes 0..11 (first free index 12).
- Automatically migrate cpp33a/cpp33b flash partition 2 while preserving the existing 64-bit console ID.
- Startup diagnostics now report PW80, C0/C1/C2 and C6/C7/C8 separately.

Expected milestone: ChuChu should leave the browser-setup retry loop and begin modem MMIO (`m1`, nonzero `rw`). Once modem traffic begins, PPP telemetry becomes the next gate.
