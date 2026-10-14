# DreamcastRecomp v1.2 cpp35a — PLUGIN_RUNTIME_V2_MAPLE_BRIDGE

This is the first checkpoint of Plugin Runtime V2.

### What changed

- ABI1 and Guest ABI1 are preserved.
- ABI2 plugins now use versioned host services instead of receiving `DCRuntime*`.
- Plugin lookup/cache is generic by device kind instead of Network-only.
- `dcr_maple` is the first ABI2 device plugin.
- Network stays on the known-good cpp34s V42/LAPM plugin.
- Maple has a safe built-in fallback if the new DLL is missing or rejected.
- New launcher option: `--rebuild-maple`.

### Deliberate limitation

cpp35a externalizes the Maple **plugin boundary and dispatch**, but not yet every Maple state field/function. It uses a transitional compatibility bridge to the proven cpp34 Maple implementation. This is intentional: it lets the next checkpoint move controller/VMU/DMA state behind ABI2 without another guest ABI change and without combining that migration with the V2-host refactor.

### Next checkpoint

Move Maple controller/VMU/DMA private state and implementation from `DreamcastRuntime.dll` into `dcr_maple.dll`, then delete the cpp35a compatibility bridge for Maple. After that, proceed to GD-ROM/CDDA.
