# cpp36b1g — Sony Haptics + Dreamcast Controller Visual V2

This release builds on cpp36b1f without changing Guest ABI1.

## Host haptics

`dcr_maple.dll` now exposes one host-haptics implementation used both by Dreamcast PuruPuru and the launcher tester. Windows backends:

- XInput (`XInputSetState`)
- Sony DualShock 4 HID (USB and Bluetooth output reports)
- Sony DualSense / PS5 HID, including DualSense Edge product ID (USB and Bluetooth output reports)

The Sony path enumerates official Sony VID `0x054C` devices and recognizes DS4 PIDs `05C4/09CC/0BA0` and DualSense PIDs `0CE6/0DF2`. Bluetooth reports include Sony CRC32 framing. DualSense Bluetooth performs the pairing-feature read used by enhanced-report stacks as a best-effort initialization step.

The launcher calls exported `dcr_maple_test_host_rumble_v1()` so a successful tester rumble exercises the same code path the game will use when a PuruPuru `SETCOND` reaches Maple.

## Visual tester V2

- more Dreamcast-like body silhouette
- original face-button layout/colors: A red, B blue, X yellow, Y green
- VMU-shaped center panel
- D-pad and stick positions moved closer to the original controller
- L/R analog pressure remains visible
- C/Z are shown only as auxiliary Maple state, not physical face buttons
- double-buffered GDI painting
- repaint only when mapped Dreamcast state actually changes
- 33 ms input/status timer while avoiding redundant redraws

## Cache / ABI

Guest ABI1 remains fingerprint `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f`. The SAFE ChuChu `dcr_game_module.dll` is unchanged. `--reuse` rebuilds only runtime/plugins because the Maple plugin changed, then auto-syncs the portable.
