# Validation — cpp34v

## Runtime evidence

Tras cpp34u, ChuChu alcanza el servidor de lobby y ejecuta una ruta post-login nueva. El nuevo bloqueo observado es:

```text
No recompiled/native target registered for Dreamcast address 0x8C036236
guest_pc=0x8C035E40
pr=0x8C035E4C
nearest-native=0x8C036118/delta=0x11E
target-bytes=22:4F:FC:7F:13:93:4C:33:33:64:32:2F:0C:D3:0B:43
```

Antes del fallo, la sesión abre `146.185.135.179:9001`, recibe el banner `DreamCast Lobby Server. Copyright SEGA Enterprises. 1999`, envía 52 bytes y recibe otros 50 bytes.

## SH-4 probe

Los 16 bytes del target fueron probados como imagen raw en `0x8C036236`:

- Reachable functions: 1
- Reachable instructions: 8
- Known SH-4: 8
- Unknown SH-4: 0
- RAW_SH4: 0

## Scope

Solo se amplía el Late-AOT overlay. Networking, ABI1, runtime layout, fast path, LAPM reliability y guest principal permanecen sin cambios.
