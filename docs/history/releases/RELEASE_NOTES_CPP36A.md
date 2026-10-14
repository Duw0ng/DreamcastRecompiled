# v1.2 cpp36a MAPLE_DEVICE_TREE

- Adds Maple Control V3 while retaining V2 Port-A compatibility.
- Removes A0-only routing from the plugin DMA path: controller ports A/B/C/D are independently addressable.
- Adds two explicit expansion slots per port; A1 remains the persistent VMU, other slots start empty.
- Port A preserves the current profile/PS4 behavior; B-D auto-map host controller devices 1-3.
- KOS/HLE Maple enumeration/status paths are port-aware.
- Adds `DCR_MAPLE_TRACE=1` and `--maple-trace`.
- Keeps the cpp35b1 frozen Guest ABI1 fingerprint; no guest recompilation is required.
- Validation: runtime/plugins build cleanly, 4-port GETCOND smoke PASS, Holly Maple IRQ PASS, 54/54 project tests PASS.
