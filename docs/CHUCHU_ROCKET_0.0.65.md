# ChuChu Rocket! checkpoint — 0.0.65

0.0.64 removes the large black rectangles in Mode Select, but the accepted Windows Options screenshot shows a second, narrower graphics failure: the yellow background, green panel, cursor, title and pre-rendered controls are visible while the dynamic option labels are blank.

## Root cause

Retail BIOS font calls return `0xA0100020`. Earlier compatibility releases mapped the surrounding 1 MiB ROM aperture so Japanese glyph reads no longer faulted, but the backing vector was initialized entirely to zero. Any retail code that dereferenced the font ROM therefore received an all-blank 1-bpp glyph.

KallistiOS documents the retail layout as 12x24 narrow characters (36 bytes each) followed by 24x24 wide JIS characters (72 bytes each). ChuChu's Options path accesses this raw font region rather than the separate KOS `bfont-fast` helper.

## 0.0.65 behavior

The guest address and layout remain unchanged. Glyph slots are synthesized lazily on first read:

- Windows maps narrow characters and compressed JIS X 0208 slots to Unicode/CP932 and rasterizes them with GDI.
- The host bitmap is repacked into the Dreamcast 1-bpp 12x24/24x24 font-ROM format before the guest read completes.
- No Dreamcast BIOS/font bytes and no font files are distributed with DreamcastRecomp.
- A deterministic procedural fallback keeps Linux/CI validation independent of host font packages.
- `bfont-rom=reads/narrow/wide` in the heartbeat exposes whether raw font-ROM reads and synthesis occur in a retail run.

The 0.0.64 translucent-list sort and all commercial code-discovery rules are carried forward unchanged. Newly collected menu crashes are intentionally not addressed in 0.0.65.
