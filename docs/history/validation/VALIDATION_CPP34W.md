# Validation — cpp34w

## Runtime evidence

Tras cpp34v, ChuChu permanece mas tiempo dentro del lobby real y recibe un payload adicional de 156 bytes desde `146.185.135.179:9001`. El nuevo bloqueo observado es:

```text
No recompiled/native target registered for Dreamcast address 0x8C036160
guest_pc=0x8C035E40
pr=0x8C035E4C
nearest-native=0x8C036118/delta=0x48
target-bytes=E6:2F:3D:9E:22:4F:4C:3E:E1:84:08:20:0A:8B:38:92
```

## SH-4 probe

Los 16 bytes del target fueron probados como imagen raw en `0x8C036160`:

- Reachable functions: 1
- Reachable instructions: 8
- Known SH-4: 8
- Unknown SH-4: 0
- RAW_SH4: 0

## Scope

Solo se amplia el Late-AOT overlay. Networking, ABI1, runtime layout, fast path, LAPM reliability y guest principal permanecen sin cambios.
