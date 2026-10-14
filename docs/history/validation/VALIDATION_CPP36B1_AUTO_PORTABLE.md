# Validation — cpp36b1 AUTO_PORTABLE_GUEST

- Guest ABI1 fingerprint: `00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f` — MATCH.
- `dc_raw_recomp --no-embed-raw-image` builds successfully.
- Synthetic raw SH-4 fixture: 1 registered function, RAW_SH4=0, embedded raw image = **0 bytes**.
- Generated `dc_image.cpp`: no `.raw_boot` payload.
- Generated runner accepts `--external-raw-image` and `--external-raw-base`.
- External image smoke: 256-byte image loaded at `0x8C010000`; guest executed and returned to host.
- Portable synchronizer smoke: folder + ZIP generated with launcher/runtime/guest/plugins/tools; no forbidden retail files.
- Core regression: tests 1-51 and 54 PASS; `generated_single_build` PASS; `generated_program_build` PASS (54/54 total).
- Maple cpp36b functionality carried forward unchanged.
