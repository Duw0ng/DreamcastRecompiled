# Portable VMU location

The ChuChu portable launcher v0.2 sets:

`DCR_SYSTEM_DATA_DIR=%LOCALAPPDATA%\DreamcastRecompiled\system`

The modular runner maps that directory to:

- `%LOCALAPPDATA%\DreamcastRecompiled\system\dreamcast_vmu_a1.bin`
- `%LOCALAPPDATA%\DreamcastRecompiled\system\dreamcast_flash.bin`

The VMU image is 128 KiB (131072 bytes). If it does not exist, it is formatted and created on first VMU access. VMU `BSYNC` persists changes. Because it is outside the portable folder, replacing or extracting a newer portable ZIP does not erase the save.
