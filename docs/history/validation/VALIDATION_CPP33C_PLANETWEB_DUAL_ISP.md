# Validation — v1.2 cpp33c PLANETWEB_DUAL_ISP

- Base: v1.2 cpp33b ISP_FLASH_DETECTION.
- `dc_raw_recomp` and all project targets compile in Release on the validation host.
- 54 CTest cases validated, including generated single-function and generated multi-function runtime builds.
- Network change is isolated to FlashROM ISP compatibility data and diagnostics; SH-4/PVR/AICA/GD-ROM logic is unchanged from cpp33b.
- Runtime commercial ChuChu test remains user-side because `ccr.cdi` is not bundled in this release.
