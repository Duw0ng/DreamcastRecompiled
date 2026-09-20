# Validation — 0.0.64

- Fresh project build and CTest: 47/47 PASS.
- Supplied ChuChu Rocket! CDI static closure: 2,776 functions / 236,519 known SH-4 instructions / 0 unknown / `RAW_SH4=0`.
- Generated commercial `dreamcast_program` compiles and links with Clang 17.
- Retail PVR diagnostics confirm foreground alpha atlases at inverse-Z ~0.286-0.333 are submitted before farther background tiles at ~0.25.
- Targeted translucent-order smoke: near alpha-zero triangle submitted first + farther opaque background submitted second; after scene sort/flush the covered pixel is the background color (`0xFF2040E0`), not the host clear color.
- Accelerated retail render probe with the sorting prototype renders the title scene correctly with its transparent artwork intact.

Windows acceptance target: verify Mode Select no longer shows the large black rectangles and confirm `pvr-tsort` activity.
