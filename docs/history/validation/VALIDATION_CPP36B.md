# cpp36b validation

## ABI

`runtime/include/dc_runtime.hpp` and `runtime/guest_abi/v1/dc_runtime.hpp` produce the same frozen ABI fingerprint:

```text
00ea21b77c60d564cf2c455c9c9d870892047aea3c3a49f28bcc844b6e04054f
```

## Maple / PuruPuru smoke

A synthetic Maple DMA list addressed Port A unit 2 with A1=VMU and A2=PuruPuru:

- PuruPuru DEVINFO: PASS (`MAPLE_FUNC_PURUPURU = 0x00010000`)
- PuruPuru SETCOND: PASS
- vibration effect parsing: PASS
- Maple DMA Holly IRQ bit 12: PASS
- VMU + PuruPuru simultaneous device tree: PASS

## Build / regression

- Runtime + `dcr_maple` + `dcr_network`: PASS
- project regression suite: effective 54/54 PASS (52 in the main run, final generated-program test passed separately after the execution time cap)
