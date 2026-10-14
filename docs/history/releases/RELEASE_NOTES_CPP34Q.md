# cpp34q — Late-AOT Target Overlay

## Trigger
cpp34p fixed the IPv4 Total Length/TCP payload bug. In the real ChuChu Rocket online path the server at `146.185.135.179:9000` then replied to the intact 60-byte client packet with `12 7B 01 C5`, after which execution reached a new indirect SH-4 target:

`0x8C036102` from `guest_pc=0x8C035E40`.

The runtime dump begins with valid decoded SH-4 instructions and the nearest registered native block is `0x8C0360E6` (delta `0x1C`), consistent with a real interior/indirect entrypoint omitted by static closure discovery.

## New approach
Instead of rebuilding the full 5185-function ChuChu guest, cpp34q adds a **Late-AOT overlay** tool:

- reads the already cached `generated/BOOTSTRAP.BIN`;
- recompiles only the missing entrypoint(s) with closure budget limited to the explicit Late-AOT roots;
- builds a tiny `dcr_guest_patch.dll` against the exact ABI1 `DreamcastRuntime`;
- installs it in the existing per-game `plugins/Release` directory;
- plugin startup registers the extra native target(s) into `DCRuntime::targets`;
- keeps a persistent `late_aot/late_target_seeds.txt` so later missing targets can be added to the same overlay.

The original generated guest, its 5185 functions, CFG, Flash and VMU are not regenerated.

## Current ChuChu target
Run:

`PATCH_CPP34Q_CHUCHU_LATE_TARGET.bat "...\ccr.cdi" 0x8C036102`

The target argument is optional for this build; it defaults to `0x8C036102`.

Expected startup line after the patch DLL is installed:

`[GUEST PATCH] cpp34q Late-AOT overlay active | targets=0x8C036102`

## ABI
No field is added to `DCRuntime`; the guest ABI remains v1 and the stable runtime header is retained.
