# cpp35b — Maple private-state extraction

cpp35b completes the second step of Plugin Runtime V2 without changing Guest ABI1.

## Ownership

When `dcr_maple` ABI2 is loaded, the plugin now owns:

- Maple MMIO registers and DMA state;
- controller presence/state and host polling;
- controller profile parsing;
- VMU A1 128 KiB flash state;
- VMU formatting, DEVINFO/GETMINFO/BREAD/BWRITE/BSYNC;
- VMU persistence path and file I/O;
- Maple descriptor/DMA telemetry.

`DreamcastRuntime.dll` keeps the historical cpp34 Maple fields only because the Guest ABI1 `DCRuntime` layout is frozen. They are compatibility/fallback storage and are not the source of truth while the ABI2 Maple plugin is loaded.

## No cpp35a bridge

`dcr_maple` no longer calls `compat_device_read32` or `compat_device_write32`. DMA reads/writes use `DCRHostServicesV2`; completion reaches Holly through `raise_irq`.

## High-level/KOS paths

The KOS/HLE Maple helpers now query the ABI2 Maple control interface, so `maple_enum_*`, `maple_dev_status`, controller profiles and host input observe the same private plugin state as low-level DMA.

## Fallback

If `dcr_maple.dll` is missing or rejected, the proven cpp34 built-in Maple implementation remains available. This is intentional during the cpp35 migration.

## VMU persistence

The plugin reads `DCR_SYSTEM_DATA_DIR` (or `DCR_SYSTEM_DATA_ROOT`) and stores A1 as:

`dreamcast_vmu_a1.bin`

Normal and portable launchers use `%LOCALAPPDATA%\DreamcastRecompiled\system`, therefore saves survive ZIP/runtime updates.
