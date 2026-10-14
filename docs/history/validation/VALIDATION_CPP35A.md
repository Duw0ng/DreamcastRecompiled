# cpp35a validation

Validated from the cpp34x source checkpoint on Linux as an architecture/regression check.

## ABI invariants

- `DCRuntime` struct body: byte-for-byte source-identical to cpp34x.
- `DCRuntime` struct SHA-256 in cpp34x and cpp35a: `9df8a6b37cc999bdeb33bc22fb0780639f8257ce461ff5110477160513548f84`.
- `runtime/include/dcr_plugin_api.hpp` (ABI1): byte-identical to cpp34x.
- Guest ABI version remains 1.

## Build

Runtime-only CMake build succeeded for:

- `DreamcastRuntime`
- `dcr_network`
- `dcr_maple`

## Plugin dispatch smoke

With `dcr_maple` present:

```text
[PLUGIN] loaded ABI2 kind=5 name=DreamcastRecompiled Maple
[PLUGIN] loaded ABI1 kind=1 name=DreamcastRecompiled Network
maple_dma=c123456
```

With `dcr_maple` removed, the same Maple register test passes through built-in fallback.

## Maple DMA smoke

A minimal Maple DEVINFO DMA list was executed through both paths.

ABI2 plugin path:

```text
plugin=1 dma=1 irq=1 state=0 resp0=0x1c210005 resp1=0x01000000
```

Fallback path:

```text
plugin=0 dma=1 irq=1 state=0 resp0=0x1c210005 resp1=0x01000000
```

Both complete DMA, set Holly ACK_A bit 12, clear Maple state and return the same controller DEVINFO packet.

## Core regression suite

CMake/CTest: **54/54 PASS**.

The suite needed two invocations only because the execution environment cut the first command during the long generated-program compilation; the resumed generated tests passed normally.
