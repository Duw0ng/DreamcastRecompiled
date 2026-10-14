# DreamcastRecompiled ChuChu Portable Audit

## Result

The legacy raw-commercial emitter is **not suitable for direct redistribution as-is** because the synthetic `.raw_boot` section spans the complete prepared retail image and `dc_image.cpp` emits every allocated byte into `dcr_game_module.dll`.

Observed source path:

- `src/disc/raw_recompiler.cpp`: `make_synthetic_elf()` creates one allocated/executable `.raw_boot` section whose size is the complete input image.
- `src/codegen/cpp_emitter.cpp`: `emit_image_cpp()` converts allocated sections into `std::array<uint8_t, N>` and `load_embedded_elf_image()` copies them into Dreamcast RAM.

That means a normal commercial guest module contains the complete prepared `BOOTSTRAP.BIN`, not only the recompiled SH-4 functions.

## Portable fix

Portable builds add `dc_raw_recomp --no-embed-raw-image`.

In this mode:

- SH-4 functions are still emitted and compiled into `dcr_game_module.dll`.
- `.raw_boot` is excluded from `dc_image.cpp`.
- `Embedded raw image` must report **0 bytes**.
- `load_embedded_elf_image()` remains present for ABI/build compatibility but contains no raw retail image.
- The user's own CDI/GDI is processed locally by `dc_disc_probe.exe` and `dc_boot_prepare.exe`.
- The locally produced `BOOTSTRAP.BIN` is supplied to `DreamcastRuntime.dll` through `DCR_EXTERNAL_RAW_IMAGE` / `DCR_EXTERNAL_RAW_BASE`. This works even with an older cached `dreamcast_program.exe` that predates the external-raw CLI.

No DCRuntime fields were added or reordered. The external image loader lives in generated runner code, so ABI1 structure layout is unchanged.

## Synthetic validation

A 256-byte raw SH-4 fixture was emitted with `--no-embed-raw-image`:

- Reachable functions: 1
- Known SH-4: 2
- Unknown SH-4: 0
- RAW_SH4: 0
- Embedded raw image: **0 bytes**

The generated `dc_image.cpp` contained no byte arrays. New generated runners still understand `--external-raw-image` / `--external-raw-base` for compatibility, while the normal portable launcher now uses the Runtime ENV path so old cached runners remain reusable.

## Distribution model

The intended tester package contains the precompiled native guest/runtime and DreamcastRecompiled-owned tooling only. It does **not** contain CDI/GDI, IP.BIN, BOOT.DISC.BIN or BOOTSTRAP.BIN. Those files are created locally from the tester's selected original game image.
