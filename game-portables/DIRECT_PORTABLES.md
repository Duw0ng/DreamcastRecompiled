# Direct game portables (one ZIP per game)

The minimal portable does **not** require the full Aqua launcher. It starts directly from `DreamcastRecompiled.exe` and uses only its matched precompiled runtime, native SH-4 module and plugins.

**[Download the exact ChuChu Rocket! direct portable](https://github.com/Duw0ng/DreamcastRecompiled/releases/tag/portable-chuchu-rocket-direct-v1.2)** — 11 files, 13.6 MiB, no CDI.

## Folder layout

```text
DreamcastRecompiled.exe
README_TESTER.txt
portable_manifest.txt
bin/
  DreamcastRuntime.dll
  dc_boot_prepare.exe
  dc_disc_probe.exe
  dcr_game_module.dll
  dreamcast_program.exe
  plugins/dcr_gdrom.dll
  plugins/dcr_maple.dll
  plugins/dcr_network.dll
```

The included game module is only for the matching title; do not rename it to run other games. The user must select an authorized, compatible game image. For ChuChu Rocket!, MK-51049 V1.007 / CHU_CHU_ROCKET_KAL is the validated disc identity.

## Reuse for future titles

Add a JSON file to `game-portables/direct-releases/` and copy the fields from [the ChuChu Rocket! manifest](direct-releases/chuchu-rocket-direct-v1.2.json). Pin the original ZIP SHA-256, specify the source and destination file mapping and a unique target release tag. The direct packager reconstructs the target archive with Go's standard ZIP writer, verifies the game ID and **requires the exact expected resulting SHA-256**. It will not attach disc images. [GitHub Actions](../.github/workflows/publish-direct-portables.yml) downloads the pinned source release, verifies, repacks and creates the standalone release with a checksum file.

For a new game, generate its expected output ZIP and hash locally with the same packager before committing the manifest; update the hash when publishing a newly verified binary set. The manifest is purposefully explicit so that one game's library cannot be silently replaced by another game's code.

**Legal note:** MIT covers original DreamcastRecompiled code, not proprietary game-derived binaries; distributors must assess the rights required. This publication model does not establish ownership or blanket redistribution permission.
