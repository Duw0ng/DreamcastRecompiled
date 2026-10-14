# Release notes — cpp37a GDROM_NATIVE_PLUGIN_PHASE1

- Adds `plugins/dcr_gdrom.dll` as ABI2 plugin kind 4.
- Adds `dcr.gdrom.control.v1` bulk control interface.
- Moves DiscMap V1/V2 parsing, track geometry, sessions, TOC generation and logical sector reads into the plugin.
- Keeps a runtime geometry mirror for not-yet-migrated CDDA/command-state code.
- Keeps the built-in GD-ROM implementation as fallback when the plugin is absent.
- Adds `DCR_GDROM_TRACE=1`, `--gdrom-trace`, `--rebuild-gdrom` and a portable launcher checkbox.
- Auto-portable sync now includes `dcr_gdrom.dll`.
- Guest ABI1 remains frozen; pinned SAFE guest unchanged.
- Existing project regression: effective 54/54 PASS.
