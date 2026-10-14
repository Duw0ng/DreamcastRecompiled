# Validation — cpp34x

## Runtime evidence

Tras cpp34w, ChuChu vuelve a alcanzar el lobby real, abre `146.185.135.179:9001`, intercambia el banner y tráfico de sesión y llega a `ppp19/ip14/tcp13`. El nuevo bloqueo es:

```text
No recompiled/native target registered for Dreamcast address 0x8C036280
guest_pc=0x8C035E40
pr=0x8C035E4C
nearest-native=0x8C036248/delta=0x38
target-bytes=E6:2F:00:E5:6C:9E:6C:91:4C:3E:4C:31:22:4F:E1:84
```

## SH-4 probe

Los 16 bytes del target fueron probados como imagen raw en `0x8C036280`:

- Reachable functions: 1
- Reachable instructions: 8
- Known SH-4: 8
- Unknown SH-4: 0
- RAW_SH4: 0

## Auto Late-AOT

`AutoLateTargetFromLogs.py` conserva los seeds existentes y puede absorber en relanzamientos futuros cualquier nuevo missing target válido encontrado en los logs recientes del mismo paquete. La promoción automática está limitada a direcciones pares `0x8C010000..0x8CFFFFFF`.

## Scope

Networking, ABI1, runtime layout, fast path, LAPM reliability y guest principal permanecen sin cambios.
