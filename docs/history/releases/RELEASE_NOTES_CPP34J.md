# cpp34j STABLE_BOOT_LAZY_FULLSTACK_ABI1

- Rebased from the exact cpp34g package.
- Guest ABI header and SH-4 emitter remain cpp34g-compatible.
- Detects/rejects cpp34h-contaminated CHUCHU_ROCKET caches and regenerates only generated/build folders.
- Full Rockwell/PPP/UDP/TCP network plugin is dormant until the first modem MMIO access.
- ChuChu console ID/checksum/mirror repair is also deferred until first modem access.
- Boot ROM writes are ignored as read-only hardware semantics without adding fields to DCRuntime.
- MSVC NOMINMAX fix retained.
