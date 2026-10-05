# Portable static-recomp audit — ChuChu Rocket! / DreamcastRecompiled

## Result

The current generated commercial guest is **not suitable for redistribution as-is** in an Unleashed-Recompiled-style package.

The raw commercial recompiler creates a synthetic executable section named `.raw_boot` containing the complete prepared bootstrap image. The C++ emitter then generates `dc_image.cpp`, which serializes every allocatable main-RAM section as literal byte arrays. For a commercial raw bootstrap this includes IP.BIN plus the game executable.

The modular guest currently links `dc_image.cpp` into `dcr_game_module.dll`, and `dcr_game_attach_v1` calls `load_embedded_elf_image(*runtime)`. Therefore the current DLL contains a verbatim copy of substantial original game data in addition to translated SH-4 code.

## Portable remediation

The portable build must:

1. Keep the precompiled SH-4 functions in `dcr_game_module.dll`.
2. Omit `dc_image.cpp` from the portable guest link.
3. Replace the call to `load_embedded_elf_image` with a loader that reads a local `BOOTSTRAP.BIN` selected through the environment variable `DCR_EXTERNAL_GAME_IMAGE`.
4. Have the launcher create that `BOOTSTRAP.BIN` locally from the tester's own CDI/GDI using `dc_disc_probe.exe` and `dc_boot_prepare.exe`.
5. Verify the extracted IP.BIN and BOOT.BIN before execution.
6. Never include CDI/GDI/IP.BIN/BOOT.BIN/BOOTSTRAP.BIN in the distributed ZIP.

This keeps the ABI unchanged: no field is added to `DCRuntime`. The existing ABI1 header remains frozen.

## Supported ChuChu Rocket! baseline

The initial portable profile targets:

- title: `CHUCHU ROCKET`
- product/version: `MK-51049 V1.007`
- IP.BIN SHA-256: `9925c18f0857ccd363cb8d641122f410083b133102ac99f06bae6bb83f9aad4d`
- BOOT.BIN SHA-256: `b43cb7977871e0c1ce7971da3da7f5a7ea4eb6903a61fbefc5a50de4230d2927`
- known CDI SHA-256: `6e95281b2feaa98e8b1327b33d8e60147c329cd314f633bba88eb25229031d3b`

Checking the extracted boot hashes, rather than only the container hash, allows a CDI or GDI carrying the same supported game revision to work.

## Known online Late-AOT entries to ship precompiled

- `0x8C036102`
- `0x8C036118`
- `0x8C036160`
- `0x8C036236`
- `0x8C036280`
- `0x8C036526`

The tester package must ship these already compiled (merged into the guest or as the precompiled ABI1 guest-patch DLL). No compiler or CMake is required on the tester's machine.

## Distribution audit gate

Before a portable ZIP is produced, the developer-side packager scans `dcr_game_module.dll` and `dcr_guest_patch.dll` for multiple 4 KiB samples from the locally prepared BOOTSTRAP.BIN. A match fails the package build. This is specifically intended to catch accidental reintroduction of the old embedded-image path.

The final ZIP must contain executable/runtime code, profiles and metadata only. Original game bytes remain local to the tester's machine.
