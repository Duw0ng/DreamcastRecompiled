# Validation — cpp36b1g

Validated in the build environment:

- Windows launcher Go source formats and cross-compiles to PE32+ x64.
- Runtime / network / Maple plugins compile on the regression host.
- `dcr_maple_test_host_rumble_v1` and `dcr_maple_probe_host_haptics_v1` are exported from the Maple plugin.
- Non-Windows haptics test path returns a controlled unsupported result.
- Frozen Guest ABI1 fingerprint remains unchanged.
- Project regression: tests 1-52 and 54 passed in the main run; generated build tests 52/53 were completed separately after the long build timeout. Effective result: **54/54 PASS**.
- Pinned SAFE ChuChu guest SHA-256 remains `ed43078f57f3120d8510d40f024f822bbd0b6059be03ef369f8d82761ae5c1f4`.

Hardware validation of DualSense HID rumble must be performed on the user's Windows PC; this environment has no attached PlayStation controller.
