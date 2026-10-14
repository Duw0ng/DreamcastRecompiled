# Validation — cpp36a MAPLE DEVICE TREE

- Runtime + `dcr_maple` + `dcr_network`: build PASS on Linux/GCC.
- Guest ABI fingerprint (mutable runtime vs frozen ABI1): `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f` on both sides.
- Maple V3 smoke: injected distinct controllers in ports A/B/C/D and issued low-level `GETCOND` DMA to 0x20/0x60/0xA0/0xE0. Every port returned its own button state.
- Maple DMA completion IRQ (Holly ACK_A bit 12): PASS.
- Maple Trace: PASS; each packet reports port/unit/device/cmd.
- Main regression suite: tests 1-52 PASS; tests 53 (`generated_program_build`) and 54 (`controller_visual_tester_package`) PASS when completed separately after build timeout. Effective project result: 54/54 PASS.
- Windows portable launcher v0.3 cross-build (`GOOS=windows GOARCH=amd64`): PASS.
