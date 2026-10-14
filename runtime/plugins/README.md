# DreamcastRecompiled Device Plugins — ABI v1

`DCR_PLUGIN_ABI_V1` is the stable device boundary introduced in cpp34a.

Current implementation:

- `dcr_network` — external modem + virtual ISP + PPP LCP/PAP/IPCP + UDP/DNS bridge.

Reserved plugin kinds:

- `DCR_PLUGIN_KIND_PVR`
- `DCR_PLUGIN_KIND_AICA`
- `DCR_PLUGIN_KIND_GDROM`
- `DCR_PLUGIN_KIND_MAPLE`

The runtime always keeps a proven built-in fallback while a subsystem is being migrated. A plugin is loaded only when its ABI and descriptor are valid. This lets device development move into small DLLs without changing/recompiling the generated SH-4 module.

For network-only work after the first Guest ABI v1 build:

```bat
run_game.bat "game.cdi" --rebuild-network
```

For host runtime work:

```bat
run_game.bat "game.cdi" --rebuild-runtime
```

Only use `--recompile-game` when SH-4/codegen/guest ABI itself changed.

## cpp35b Maple

`dcr_maple` is now a true ABI2 private-state device: controller, VMU and Maple DMA/MMIO state live inside the plugin. It uses host memory/IRQ services and no longer calls the cpp35a compatibility bridge.

## cpp37b GD-ROM

`dcr_gdrom` keeps the cpp37a `dcr.gdrom.control.v1` media API and adds host-only `dcr.gdrom.control.v2`. V2 owns request IDs/status/results, multi-transfer progress, callback state and G1/GD DMA bulk copies/IRQ signaling. CDDA decoding/playback remains in DreamcastRuntime until cpp37c. Guest ABI1 is unchanged.
## cpp37c GD-ROM

`dcr_gdrom` now exposes host-only `dcr.gdrom.control.v3`. V3 completes the device extraction by moving Red-Book CDDA transport and decode state into the plugin: PLAY/PAUSE/RELEASE/STOP/SEEK, raw 2352-byte audio reads, track/FAD resolution, byte-order analysis, Yellow-Book compatibility and PCM generation. DreamcastRuntime keeps only scheduling, IRQ/audio integration glue and fallback. PCM is delivered in 64-frame batches. Guest ABI1 remains unchanged.

