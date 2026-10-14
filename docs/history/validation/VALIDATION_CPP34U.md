# Validation — cpp34u

## Runtime evidence

El nuevo bloqueo observado tras aceptar la pantalla de nickname/password fue:

```text
No recompiled/native target registered for Dreamcast address 0x8C036526
nearest-native=0x8C0365E2/delta=0xBC
target-bytes=E6:2F:43:6E:29:94:1B:D2:EC:34:1B:D1:22:4F:41:53
```

## SH-4 probe

Los 16 bytes del target fueron probados como imagen raw en `0x8C036526`:

- Reachable instructions: 8
- Known SH-4: 8
- Unknown SH-4: 0
- RAW_SH4: 0

## Scope

Solo se amplía el Late-AOT overlay. Networking, ABI1, runtime layout y guest principal permanecen sin cambios.
