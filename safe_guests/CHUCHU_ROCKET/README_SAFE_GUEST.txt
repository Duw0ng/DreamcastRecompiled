DreamcastRecompiled - ChuChu Rocket SAFE portable guest seed

This dcr_game_module.dll was supplied from the previous ChuChu online portable build.
Validated for cpp36b1 adoption:
- PE32+ x86-64 DLL
- exports dcr_game_abi_version_v1 and dcr_game_attach_v1
- dcr_game_abi_version_v1 returns 0x00010000 (Guest ABI1)
- known cpp34x Late-AOT target immediates are present
- SHA-256 is pinned in manifest.ini
- source portable builder generated guests with --no-embed-raw-image

Do not edit or replace this DLL without updating manifest.ini.
