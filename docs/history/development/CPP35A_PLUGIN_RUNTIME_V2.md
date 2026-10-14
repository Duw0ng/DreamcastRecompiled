# cpp35a — Plugin Runtime V2 + Maple boundary

cpp35a starts the large modular-runtime refactor without changing Guest ABI1.

## Stable boundary

`DCRuntime` remains byte-layout compatible with cpp34x. ABI2 plugins do **not** receive a `DCRuntime*`; they receive a versioned `DCRHostServicesV2` table and keep plugin-owned instance state returned by `create()`.

```text
dcr_game_module.dll (Guest ABI1)
          |
          v
DreamcastRuntime.dll
  CPU / RAM / IRQ / scheduler / plugin host
          |
          +-- ABI1: dcr_network.dll  (cpp34s, unchanged)
          |
          +-- ABI2: dcr_maple.dll
```

The host prefers `dcr_plugin_query_v2` and falls back to `dcr_plugin_query_v1`. ABI1 remains byte-identical to cpp34x.

## ABI2 host services

The new table exposes coarse host services instead of growing `DCRuntime`:

- `read8/read16/read32`, `write8/write16/write32`
- `get_memory_region`
- `raise_irq/clear_irq`
- `schedule_event` (reserved in cpp35a)
- `log`
- `get_config`
- transitional `compat_device_read32/write32`

ABI2 also adds 32-bit MMIO hooks and `query_interface`, so later PVR/AICA/GD-ROM plugins can expose device-specific fast-path interfaces without changing the base descriptor.

## Why Maple is a bridge in cpp35a

Moving controller polling, VMU persistence, Maple packet parsing and DMA state all at once would mix an architecture change with a behavior rewrite. cpp35a first makes the **dispatch boundary real and replaceable**: Maple MMIO goes through `dcr_maple.dll` when present, and automatically falls back to the exact cpp34 implementation when absent.

The temporary compatibility callbacks do not expose `DCRuntime*` across ABI2. They are intentionally marked transitional. The next Maple extraction can move state/functions behind the same ABI2 boundary without touching the guest module again.

## Rebuild workflow

```bat
run_game.bat "juego.cdi" --rebuild-maple
run_game.bat "juego.cdi" --rebuild-network
run_game.bat "juego.cdi" --rebuild-runtime
```

`--rebuild-maple` builds only the Maple plugin and leaves the persistent SH-4 guest module untouched.
